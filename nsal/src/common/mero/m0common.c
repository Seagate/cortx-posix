/*
 * Filename:         m0common.c
 * Description:      Contains mero helpers

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 @todo Move this code to utils/src/eos/helpers.c file, after
 iterators are generalized. (EOS-4351)
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syscall.h> /* for gettid */
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <dirent.h>

#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"
#include "lib/thread.h"
#include "m0common.h"
#include <motr/helpers/helpers.h>

/* Key Iterator */

struct m0_key_iter_priv {
	struct m0_bufvec key;
	struct m0_bufvec val;
	struct m0_clovis_op *op;
	int rcs[1];
	bool initialized;
};

_Static_assert(sizeof(struct m0_key_iter_priv) <=
	       sizeof(((struct kvstore_iter*) NULL)->priv),
	       "m0_key_iter_priv does not fit into 'priv'");

static inline
struct m0_key_iter_priv *m0_key_iter_priv(struct kvstore_iter *iter)
{
	return (void *) &iter->priv[0];
}

void m0_key_iter_fini(struct kvstore_iter *iter)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);

	if (!priv->initialized)
		goto out;

	m0_bufvec_free(&priv->key);
	m0_bufvec_free(&priv->val);

	if (priv->op) {
		m0_clovis_op_fini(priv->op);
		m0_clovis_op_free(priv->op);
	}

out:
	return;
}

bool m0_key_iter_find(struct kvstore_iter *iter, const void* prefix,
		      size_t prefix_len)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_bufvec *key = &priv->key;
	struct m0_bufvec *val = &priv->val;
	struct m0_clovis_op **op = &priv->op;
	struct m0_clovis_idx *index = iter->idx.index_priv;
	int rc;

	if (prefix_len == 0)
		rc = m0_bufvec_empty_alloc(key, 1);
	else
		rc = m0_bufvec_alloc(key, 1, prefix_len);
	if (rc != 0) {
		goto out;
	}

	rc = m0_bufvec_empty_alloc(val, 1);
	if (rc != 0) {
		goto out_free_key;
	}

	memcpy(priv->key.ov_buf[0], prefix, prefix_len);

	rc = m0_clovis_idx_op(index, M0_CLOVIS_IC_NEXT, &priv->key, &priv->val,
			      priv->rcs, 0, op);

	if (rc != 0) {
		goto out_free_val;
	}


	m0_clovis_op_launch(op, 1);
	rc = m0_clovis_op_wait(*op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);

	if (rc != 0) {
		goto out_free_op;
	}

	if (priv->rcs[0] != 0) {
		goto out_free_op;
	}

	/* release objects back to priv */
	key = NULL;
	val = NULL;
	op = NULL;
	priv->initialized = true;

out_free_op:
	if (op && *op) {
		m0_clovis_op_fini(*op);
		m0_clovis_op_free(*op);
	}

out_free_val:
	if (val)
		m0_bufvec_free(val);
out_free_key:
	if (key)
		m0_bufvec_free(key);
out:
	if (rc != 0) {
		memset(&priv, 0, sizeof(*priv));
	}

	iter->inner_rc = rc;

	return rc == 0;
}

/** Make a non-empty bufvec to be an empty bufvec.
 * Frees internal buffers (to data) inside the bufvec
 * without freeing m0_bufvec::ov_buf and m0_bufvec::ov_bec::v_count.
 */
static void m0_bufvec_free_data(struct m0_bufvec *bufvec)
{
	uint32_t i;

	assert(bufvec->ov_buf);

	for (i = 0; i < bufvec->ov_vec.v_nr; ++i) {
		m0_free(bufvec->ov_buf[i]);
		bufvec->ov_buf[i] = NULL;
	}
}

bool m0_key_iter_next(struct kvstore_iter *iter)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_clovis_idx *index = iter->idx.index_priv;
	bool can_get_next = false;

	assert(priv->initialized);

	/* Clovis API: "'vals' vector ... should contain NULLs" */
	m0_bufvec_free_data(&priv->val);

	iter->inner_rc = m0_clovis_idx_op(index, M0_CLOVIS_IC_NEXT,
					  &priv->key, &priv->val, priv->rcs,
					  M0_OIF_EXCLUDE_START_KEY,  &priv->op);

	if (iter->inner_rc != 0) {
		goto out;
	}

	m0_clovis_op_launch(&priv->op, 1);
	iter->inner_rc = m0_clovis_op_wait(priv->op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);

	if (iter->inner_rc != 0) {
		goto out;
	}

	iter->inner_rc = priv->rcs[0];

	if (iter->inner_rc == 0)
		can_get_next = true;

out:
	return can_get_next;
}

size_t m0_key_iter_get_key(struct kvstore_iter *iter, void **buf)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_bufvec *v = &priv->key;
	*buf = v->ov_buf[0];
	return v->ov_vec.v_count[0];
}

size_t m0_key_iter_get_value(struct kvstore_iter *iter, void **buf)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_bufvec *v = &priv->val;
	*buf = v->ov_buf[0];
	return v->ov_vec.v_count[0];
}

