/*
 * Filename:         dstore_bufvec.c
 * Description:      IO Vectors and buffers (implementation).

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 *
 * This file contains implementation public API functions for
 * IO Vector and IO Buffer data types.
 */
#include "dstore_bufvec.h"
#include <stdlib.h> /* malloc */
#include <errno.h> /* errno values */
#include "../dstore/dstore_internal.h"
#include "debug.h" /* dassert */

int dstore_io_buf_init(void *data, size_t size, size_t offset,
		      struct dstore_io_buf **buf)
{
	struct dstore_io_buf *result;
	int rc = 0;

	dassert(data != NULL);

	result = malloc(sizeof(struct dstore_io_buf));
	if (result == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	result->buf = data;
	result->size = size;
	result->offset = offset;

	*buf = result;

out:
	dassert((!(*buf)) || dstore_io_buf_invariant(*buf));
	return rc;
}

void dstore_io_buf_fini(struct dstore_io_buf *buf)
{
	if (buf) {
		dassert(dstore_io_buf_invariant(buf));
		free(buf);
	}
}

/* FIXME:
 * The corresponding comment for this function claims that the function
 * does zerocopy conversion, however the actual implementation is not
 * really cheap because causes calloc/free calls.
 * Although this function does not copy any user-provided IO data,
 * it still may cause performance degradation.
 * This issue will be addressed in EOS-8622.
 */
int dstore_io_buf2vec(struct dstore_io_buf **buf, struct dstore_io_vec **vec)
{
	struct dstore_io_vec *result;
	int rc = 0;

	dassert(buf);
	dassert(*buf);
	dassert(vec);
	dassert(dstore_io_buf_invariant(*buf));

	result = calloc(1, sizeof(struct dstore_io_vec));
	if (result == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	result->edbuf = **buf;
	dstore_io_buf_fini(*buf);
	*buf = NULL;

	dstore_io_vec_set_from_edbuf(result);
	*vec = result;

out:
	dassert((!(*vec)) || (*buf == NULL && dstore_io_vec_invariant(*vec)));
	return rc;
}

void dstore_io_vec_fini(struct dstore_io_vec *vec)
{
	if (vec) {
		dassert(dstore_io_vec_invariant(vec));
		free(vec);
	}
}

