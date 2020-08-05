/*
 * Filename:         eos_kvstore.c
 * Description:      Implementation of EOS KVStore.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains APIs which implement NSAL's KVStore framework,
 on top of eos clovis index APIs.
*/
#include "internal/eos/eos_kvstore.h"
#include <eos/helpers.h>

int eos_kvs_init(struct collection_item *cfg_items)
{
	return m0init(cfg_items);
}

int eos_kvs_fini(void)
{
	m0fini();
	return 0;
}

int eos_kvs_alloc(void **ptr, size_t size)
{
	*ptr = m0kvs_alloc(size);
	if (*ptr == NULL)
		return -ENOMEM;

	return 0;
}

void eos_kvs_free(void *ptr)
{
	return m0kvs_free(ptr);
}


int eos_kvs_index_create(const kvs_idx_fid_t *fid, struct kvs_idx *index)
{
        int rc;
	struct m0_uint128 mfid = M0_UINT128(0, 0);
	struct m0_clovis_idx *idx = NULL;
	kvs_idx_fid_t gfid;

	index->index_priv = NULL;

	if (fid == NULL) {
		const char *vfid_str = eos_kvs_get_gfid();
	        rc = eos_kvs_fid_from_str(vfid_str, &gfid);

		index->index_fid = gfid;

		mfid.u_hi = gfid.f_hi;
		mfid.u_lo = gfid.f_lo;

	} else {
		index->index_fid = *fid;

		mfid.u_hi = fid->f_hi;
		mfid.u_lo = fid->f_lo;

	}


        rc = m0idx_create(&mfid, &idx);
        if (rc != 0) {
                fprintf(stderr, "Failed to create index, fid=%" PRIx64 ":%" PRIx64 "\n",
			mfid.u_hi, mfid.u_lo);
		goto out;
	}

	/** Use KVStore index's priv to track Mero clovis index */
	index->index_priv = idx;
out:
        return rc;
}

int eos_kvs_index_delete(const kvs_idx_fid_t *fid)
{
        int rc;
	struct m0_uint128 mfid = M0_UINT128(0, 0);

	mfid.u_hi = fid->f_hi;
	mfid.u_lo = fid->f_lo;

        rc = m0idx_delete(&mfid);
        if (rc != 0) {
                fprintf(stderr, "Failed to delete index, fid=%" PRIx64 ":%" PRIx64 "\n",
			mfid.u_hi, mfid.u_lo);
	}

        return rc;
}

int eos_kvs_index_open(const kvs_idx_fid_t *fid, struct kvs_idx *index)
{
	int rc;
	struct m0_uint128 mfid = M0_UINT128(0, 0);
	struct m0_clovis_idx *idx = NULL;
	kvs_idx_fid_t gfid;

	index->index_priv = NULL;

	if (fid == NULL) {
		const char *vfid_str = eos_kvs_get_gfid();

	        rc = eos_kvs_fid_from_str(vfid_str, &gfid);

		index->index_fid = gfid;

		mfid.u_hi = gfid.f_hi;
		mfid.u_lo = gfid.f_lo;

	} else {
		index->index_fid = *fid;

		mfid.u_hi = fid->f_hi;
		mfid.u_lo = fid->f_lo;
	}

	rc = m0idx_open(&mfid, &idx);
	if (rc != 0) {
		fprintf(stderr, "Failed to open index, fid=%" PRIx64 ":%" PRIx64 "\n",
			mfid.u_hi, mfid.u_lo);
		goto out;
	}

	index->index_priv = idx;
out:
	return rc;
}

int eos_kvs_index_close(struct kvs_idx *index)
{
	struct m0_clovis_idx *idx = (struct m0_clovis_idx *)index->index_priv;

        m0idx_close(idx);

	index->index_priv = NULL;
	index->index_fid.f_hi = 0;
	index->index_fid.f_lo = 0;

	return 0;
}

int eos_kvs_begin_transaction(struct kvs_idx *index)
{
	return 0;
}

int eos_kvs_end_transaction(struct kvs_idx *index)
{
	return 0;
}

int eos_kvs_discard_transaction(struct kvs_idx *index)
{
	return 0;
}

int eos_kvs_get_bin(struct kvs_idx *index, void *k, const size_t klen,
		 void **v, size_t *vlen)
{
	return m0kvs_get(index->index_priv, k, klen, v, vlen);
}

int eos_kvs4_get_bin(void *k, const size_t klen, void **v, size_t *vlen)
{
	return m0kvs4_get(k, klen, v, vlen);
}

int eos_kvs_set_bin(struct kvs_idx *index, void *k, const size_t klen,
		 void *v, const size_t vlen)
{
	return m0kvs_set(index->index_priv, k, klen, v, vlen);
}

int eos_kvs4_set_bin(void *k, const size_t klen, void *v, const size_t vlen)
{
	return m0kvs4_set(k, klen, v, vlen);
}

int eos_kvs_del_bin(struct kvs_idx *index, const void *key, size_t klen)
{
	return m0kvs_del(index->index_priv, (void *) key, klen);
}

int eos_kvs_gen_fid(kvs_idx_fid_t *index_fid)
{
	return m0kvs_idx_gen_fid((struct m0_uint128 *) index_fid);
}

const char *eos_kvs_get_gfid(void)
{
	return m0_get_gfid();
}

int eos_kvs_fid_from_str(const char *fid_str, kvs_idx_fid_t *out_fid)
{
	return m0_fid_sscanf(fid_str, (struct m0_fid *) out_fid);
}

bool get_list_cb_size(char *k, void *arg)
{
	int size;

	memcpy((char *)&size, (char *)arg, sizeof(int));
	size += 1;
	memcpy((char *)arg, (char *)&size, sizeof(int));

	return true;
}

int eos_kvs_get_list_size(void *ctx, char *pattern, size_t plen)
{
	char initk[KLEN];
	int size = 0;
	int rc;

	strcpy(initk, pattern);
	initk[plen - 2] = '\0';

	rc = m0kvs_pattern(ctx, initk, pattern,
			   get_list_cb_size, &size);
	if (rc < 0)
		return rc;

	return size;
}

/******************************************************************************/
/* Key Iterator */

_Static_assert(sizeof(struct m0kvs_key_iter) <=
	       sizeof(((struct kvs_itr*) NULL)->priv),
	       "m0kvs_key_iter does not fit into 'priv'");

static inline
struct m0kvs_key_iter *eos_key_iter_priv(struct kvs_itr *iter)
{
	return (void *) &iter->priv[0];
}

void eos_kvs_iter_get_kv(struct kvs_itr *iter, void **key, size_t *klen,
                          void **val, size_t *vlen)
{
	struct m0kvs_key_iter *priv = eos_key_iter_priv(iter);
	return m0kvs_key_iter_get_kv(priv, key, klen, val, vlen);
}

int eos_kvs_prefix_iter_has_prefix(struct kvs_itr *iter)
{
	void *key, *value;
	size_t key_len, val_len;
	int rc = 0;

	eos_kvs_iter_get_kv(iter, &key, &key_len, &value, &val_len);
	assert(key_len >= iter->prefix.len);
	rc = memcmp(iter->prefix.buf, key, iter->prefix.len);
	if (rc != 0) {
		rc = -ENOENT;
	}
	return rc;
}

/** Find the first record following by the prefix and set iter to it.
 * @param iter Iterator object to initialized with the starting record.
 * @param prefix Key prefix to be found.
 * @param prefix_len Length of the prefix.
 * @return True if found, otherwise False. @see kvs_itr::inner_rc for return
 * code.
 */
int eos_kvs_prefix_iter_find(struct kvs_itr *iter)
{
	int rc = 0;
	struct m0kvs_key_iter *priv = NULL;

	priv = eos_key_iter_priv(iter);
	priv->index = iter->idx.index_priv;
	rc = m0kvs_key_iter_find(iter->prefix.buf, iter->prefix.len, priv);
	iter->inner_rc = rc;
	if (rc) {
		goto out;
	}
	rc = eos_kvs_prefix_iter_has_prefix(iter);
out:
	return rc;
}

/* Find the next record and set iter to it. */
int eos_kvs_prefix_iter_next(struct kvs_itr *iter)
{
	int rc = 0;
	struct m0kvs_key_iter *priv = eos_key_iter_priv(iter);
	rc = m0kvs_key_iter_next(priv);
	iter->inner_rc = rc;
	if (rc) {
		goto out;
	}
	rc = eos_kvs_prefix_iter_has_prefix(iter);
out:
	return rc;
}

/** Cleanup key iterator object */
void eos_kvs_prefix_iter_fini(struct kvs_itr *iter)
{
	struct m0kvs_key_iter *priv = eos_key_iter_priv(iter);
	m0kvs_key_iter_fini(priv);
}

struct kvstore_ops eos_kvs_ops = {
	.init = eos_kvs_init,
	.fini = eos_kvs_fini,
	.alloc = eos_kvs_alloc,
	.free = eos_kvs_free,

	.begin_transaction = eos_kvs_begin_transaction,
	.end_transaction = eos_kvs_end_transaction,
	.discard_transaction = eos_kvs_discard_transaction,

	.index_create = eos_kvs_index_create,
	.index_delete = eos_kvs_index_delete,
	.index_open = eos_kvs_index_open,
	.index_close = eos_kvs_index_close,
	.index_gen_fid = eos_kvs_gen_fid,

	.get_bin = eos_kvs_get_bin,
	.get4_bin = eos_kvs4_get_bin,
	.set_bin = eos_kvs_set_bin,
	.del_bin = eos_kvs_del_bin,

	.kv_find = eos_kvs_prefix_iter_find,
	.kv_next = eos_kvs_prefix_iter_next,
	.kv_fini = eos_kvs_prefix_iter_fini,
	.kv_get = eos_kvs_iter_get_kv
};

int eos_kvs_list_set(struct kvs_idx *index,
                     const struct kvgroup *kv_grp)
{
	struct m0kvs_list key, val;
	int rc = 0, i;

	rc = m0kvs_list_alloc(&key, kv_grp->kv_count);
	if (rc != 0) {
		goto out;
	}
	rc = m0kvs_list_alloc(&val, kv_grp->kv_count);
	if (rc != 0) {
		goto out_free_key;
	}

	for (i = 0; i < kv_grp->kv_count; ++i) {
		m0kvs_list_add(&key, kv_grp->kv_list[i]->key.buf,
		               kv_grp->kv_list[i]->key.len, i);
		m0kvs_list_add(&val, kv_grp->kv_list[i]->val.buf,
		               kv_grp->kv_list[i]->val.len, i);
	}

	rc = m0kvs_list_set(index->index_priv, &key, &val);

	m0kvs_list_free(&val);
out_free_key:
	m0kvs_list_free(&key);
out:
	return rc;
}

int eos_kvs_list_get(struct kvs_idx *index,
                     struct kvgroup *kv_grp)
{
	struct m0kvs_list key, val;
	int rc = 0, i;

	rc = m0kvs_list_alloc(&key, kv_grp->kv_count);
	if (rc != 0) {
		goto out;
	}
	rc = m0kvs_list_alloc(&val, kv_grp->kv_count);
	if (rc != 0) {
		goto out_free_key;
	}

	for (i = 0; i < kv_grp->kv_count; ++i) {
		m0kvs_list_add(&key, kv_grp->kv_list[i]->key.buf,
		               kv_grp->kv_list[i]->key.len, i);
	}

	rc = m0kvs_list_get(index->index_priv, &key, &val);

	m0kvs_list_free(&val);
out_free_key:
	m0kvs_list_free(&key);
out:
	return rc;
}
