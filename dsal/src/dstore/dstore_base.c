/*
 * Filename:         dstore_base.c
 * Description:      Contains implementation of basic dstore
 *                   framework APIs.
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 */

#include <stdlib.h>
#include "dstore.h"
#include <assert.h> /* TODO: to be replaced with dassert() */
#include <errno.h> /* ret codes such as EINVAL */
#include <string.h> /* strncmp, strlen */
#include <ini_config.h> /* collection_item and related functions */
#include "common/helpers.h" /* RC_WRAP* */
#include "common/log.h" /* log_* */
#include "debug.h" /* dassert */
#include "dstore_internal.h" /* import internal API definitions */
#include "dstore_bufvec.h" /* data buffers and vectors */

static struct dstore g_dstore;

struct dstore *dstore_get(void)
{
	return &g_dstore;
}

struct dstore_module {
	char *type;
	const struct dstore_ops *ops;
};

static struct dstore_module dstore_modules[] = {
	{ "cortx", &cortx_dstore_ops },
	{ NULL, NULL },
};

int dstore_init(struct collection_item *cfg, int flags)
{
	int rc;
	struct dstore *dstore = dstore_get();
	struct collection_item *item = NULL;
	const struct dstore_ops *dstore_ops = NULL;
	char *dstore_type = NULL;
	int i;

	assert(dstore && cfg);

	RC_WRAP(get_config_item, "dstore", "type", cfg, &item);
	if (item == NULL) {
		fprintf(stderr, "dstore type not specified\n");
		return -EINVAL;
	}

	dstore_type = get_string_config_value(item, NULL);

	assert(dstore_type != NULL);

	for (i = 0; dstore_modules[i].type != NULL; ++i) {
		if (strncmp(dstore_type, dstore_modules[i].type,
		    strlen(dstore_type)) == 0) {
			dstore_ops = dstore_modules[i].ops;
			break;
		}
	}

	dstore->type = dstore_type;
	dstore->cfg = cfg;
	dstore->flags = flags;
	dstore->dstore_ops = dstore_ops;
	assert(dstore->dstore_ops != NULL);

	assert(dstore->dstore_ops->init != NULL);
	rc = dstore->dstore_ops->init(cfg);
	if (rc) {
		return rc;
	}

	return 0;
}

int dstore_fini(struct dstore *dstore)
{
	assert(dstore && dstore->dstore_ops && dstore->dstore_ops->fini);

	return dstore->dstore_ops->fini();
}

int dstore_obj_create(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid)
{
	assert(dstore && dstore->dstore_ops && oid &&
	       dstore->dstore_ops->obj_create);

	return dstore->dstore_ops->obj_create(dstore, ctx, oid);
}

int dstore_obj_delete(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid)
{
	assert(dstore && dstore->dstore_ops && oid &&
	       dstore->dstore_ops->obj_delete);

	return dstore->dstore_ops->obj_delete(dstore, ctx, oid);
}

int dstore_obj_read(struct dstore *dstore, void *ctx,
		    dstore_oid_t *oid, off_t offset,
		    size_t buffer_size, void *buffer, bool *end_of_file,
		    struct stat *stat)
{
	assert(dstore && oid && buffer && buffer_size && stat &&
		dstore->dstore_ops->obj_read);

	return dstore->dstore_ops->obj_read(dstore, ctx, oid, offset,
					    buffer_size, buffer, end_of_file,
					    stat);
}

int dstore_obj_write(struct dstore *dstore, void *ctx,
		     dstore_oid_t *oid, off_t offset,
		     size_t buffer_size, void *buffer, bool *fsal_stable,
		     struct stat *stat)
{
	assert(dstore && oid && buffer && buffer_size && stat &&
		dstore->dstore_ops->obj_write);

	return dstore->dstore_ops->obj_write(dstore, ctx, oid, offset,
					     buffer_size, buffer, fsal_stable,
					     stat);
}

int dstore_obj_resize(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid,
		      size_t old_size, size_t new_size)
{
	assert(dstore && oid &&
	       dstore->dstore_ops && dstore->dstore_ops->obj_resize);

	return dstore->dstore_ops->obj_resize(dstore, ctx, oid, old_size,
					      new_size);
}

int dstore_get_new_objid(struct dstore *dstore, dstore_oid_t *oid)
{
	assert(dstore && oid && dstore->dstore_ops &&
	       dstore->dstore_ops->obj_get_id);

	return dstore->dstore_ops->obj_get_id(dstore, oid);
}

int dstore_obj_open(struct dstore *dstore,
		    const dstore_oid_t *oid,
		    struct dstore_obj **out)
{
	int rc;
	struct dstore_obj *result = NULL;

	dassert(dstore);
	dassert(oid);
	dassert(out);

	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->obj_open, dstore, oid,
		      &result);

	result->ds = dstore;
	result->oid = *oid;

	/* Transfer the ownership of the created object to the caller. */
	*out = result;
	result = NULL;

out:
	if (result) {
		dstore_obj_close(result);
	}

	log_debug("open " OBJ_ID_F ", %p, rc=%d", OBJ_ID_P(oid),
		  rc == 0 ? *out : NULL, rc);
	return rc;
}

int dstore_obj_close(struct dstore_obj *obj)
{
	int rc;
	struct dstore *dstore;

	dassert(obj);
	dstore = obj->ds;
	dassert(dstore);

	log_trace("close >>> " OBJ_ID_F ", %p",
		  OBJ_ID_P(dstore_obj_id(obj)), obj);

	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->obj_close, obj);

out:
	log_trace("close <<< (%d)", rc);
	return rc;
}

static int dstore_io_op_init_and_submit(struct dstore_obj *obj,
                                        struct dstore_io_vec *bvec,
                                        dstore_io_op_cb_t cb,
                                        void *cb_ctx,
                                        struct dstore_io_op **out,
                                        enum dstore_io_op_type op_type)
{
	int rc;
	struct dstore *dstore;
	struct dstore_io_op *result = NULL;

	dassert(obj);
	dassert(obj->ds);
	dassert(bvec);
	dassert(out);
	dassert(dstore_obj_invariant(obj));
	dassert(dstore_io_vec_invariant(bvec));
	/* Only WRITE/READ is supported so far */
	dassert(op_type == DSTORE_IO_OP_WRITE ||
		op_type == DSTORE_IO_OP_READ);

	dstore = obj->ds;

	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->io_op_init, obj,
		      op_type, bvec, cb, cb_ctx, &result);
	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->io_op_submit, result);

	*out = result;
	result = NULL;

out:
	if (result) {
		dstore->dstore_ops->io_op_fini(result);
	}

	dassert((!(*out)) || dstore_io_op_invariant(*out));
	return rc;
}

int dstore_io_op_write(struct dstore_obj *obj,
                       struct dstore_io_vec *bvec,
                       dstore_io_op_cb_t cb,
                       void *cb_ctx,
                       struct dstore_io_op **out)
{
	int rc;

	rc = dstore_io_op_init_and_submit(obj, bvec, cb, cb_ctx, out,
					  DSTORE_IO_OP_WRITE);

	log_debug("write (" OBJ_ID_F " <=> %p, "
		  "vec=%p, cb=%p, ctx=%p, *out=%p) rc=%d",
		  OBJ_ID_P(dstore_obj_id(obj)), obj,
		  bvec, cb, cb_ctx, rc == 0 ? *out : NULL, rc);

	return rc;
}

int dstore_io_op_read(struct dstore_obj *obj, struct dstore_io_vec *bvec,
                      dstore_io_op_cb_t cb, void *cb_ctx,
                      struct dstore_io_op **out)
{
	int rc;

	rc = dstore_io_op_init_and_submit(obj, bvec, cb, cb_ctx, out,
					  DSTORE_IO_OP_READ);

	log_debug("read (" OBJ_ID_F " <=> %p, "
		  "vec=%p, cb=%p, ctx=%p, *out=%p) rc=%d",
		  OBJ_ID_P(dstore_obj_id(obj)), obj,
		  bvec, cb, cb_ctx, rc == 0 ? *out : NULL, rc);

	return rc;
}

int dstore_io_op_wait(struct dstore_io_op *op)
{
	int rc = 0;
	struct dstore *dstore;

	dassert(op);
	dassert(op->obj);
	dassert(op->obj->ds);
	dassert(dstore_io_op_invariant(op));

	dstore = op->obj->ds;

	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->io_op_wait, op);

out:
	log_debug("wait (" OBJ_ID_F " <=> %p, op=%p) rc=%d",
		  OBJ_ID_P(dstore_obj_id(op->obj)), op->obj, op, rc);
	return rc;
}

void dstore_io_op_fini(struct dstore_io_op *op)
{
	struct dstore *dstore;

	dassert(op);
	dassert(op->obj);
	dassert(op->obj->ds);
	dassert(dstore_io_op_invariant(op));

	dstore = op->obj->ds;

	log_trace("fini >>> (" OBJ_ID_F " <=> %p, op=%p)",
		  OBJ_ID_P(dstore_obj_id(op->obj)), op->obj, op);

	dstore->dstore_ops->io_op_fini(op);

	log_trace("%s", (char *) "fini <<< ()");
}

static int pwrite_aligned(struct dstore_obj *obj, char *write_buf,
			  size_t buf_size, off_t offset, dstore_io_op_cb_t cb,
			  void *cb_ctx)
{
	int rc = 0;

	dassert(obj);
        dassert(write_buf);
        dassert(offset >= 0);

        struct dstore_io_op *wop = NULL;
        struct dstore_io_vec *data = NULL;
        struct dstore_io_buf *buf = NULL;

	RC_WRAP_LABEL(rc, out, dstore_io_buf_init, write_buf, buf_size,
		      offset, &buf);

	RC_WRAP_LABEL(rc, out, dstore_io_buf2vec, &buf, &data);

	RC_WRAP_LABEL(rc, out, dstore_io_op_write, obj, data, cb, cb_ctx, &wop);

	RC_WRAP_LABEL(rc, out, dstore_io_op_wait, wop);

out:
	if (wop) {
		dstore_io_op_fini(wop);
	}

	if (data) {
		dstore_io_vec_fini(data);
	}

	if (buf) {
		dstore_io_buf_fini(buf);
	}

	log_trace("pwrite_aligned: rc=%d", rc);

	return rc;
}

static int pread_aligned(struct dstore_obj *obj, char *read_buf,
			 size_t buf_size, off_t offset, dstore_io_op_cb_t cb,
			 void *cb_ctx)
{
	int rc = 0;

	dassert(obj);
	dassert(read_buf);
	dassert(offset >= 0);

        struct dstore_io_op *rop = NULL;
        struct dstore_io_vec *data = NULL;
        struct dstore_io_buf *buf = NULL;

        RC_WRAP_LABEL(rc, out, dstore_io_buf_init, read_buf, buf_size,
                      offset, &buf);

        RC_WRAP_LABEL(rc, out, dstore_io_buf2vec, &buf, &data);

        RC_WRAP_LABEL(rc, out, dstore_io_op_read, obj, data, cb, cb_ctx, &rop);

        RC_WRAP_LABEL(rc, out, dstore_io_op_wait, rop);

out:
        if (rop) {
                dstore_io_op_fini(rop);
        }

        if (data) {
                dstore_io_vec_fini(data);
        }

        if (buf) {
                dstore_io_buf_fini(buf);
        }

	log_trace("pread_aligned: rc=%d", rc);

        return rc;
}

static
int pread_aligned_handle_holes(struct dstore_obj *obj, char *read_buf,
			       size_t buf_size, off_t offset, size_t bs,
			       dstore_io_op_cb_t cb, void *cb_ctx)
{
	int rc = 0;

	rc = pread_aligned(obj, read_buf, buf_size, offset, cb, cb_ctx);

	/* The following logic handle two case which are explained below
	 * 1. Motr is not able to handle the case where some part of object
	 * have not been written or created. For that it returns -ENOENT
	 * even though some of them are available and we should get valid data
	 * for them atleast. For such case, this is the workaround where
	 * if we are reading more than one block size we will read
	 * all the block one by one so that for originally available
	 * block we will get proper data.
	 * 2. In case of sparse block, the block which is not written will be
	 * filled with all zeros.
	*/
	if (rc == -ENOENT)
	{
		int count = buf_size/bs;
		int i;

		for (i = 0; i < count; i++)
		{
			/* read block one by one */
			rc = pread_aligned(obj, read_buf+(i*bs), bs,
					   offset+(i*bs), cb, cb_ctx);

			if (rc != 0)
			{
				if (rc == -ENOENT)
				{
					memset(read_buf + (i*bs), 0, bs);
				}
				else
				{
					log_err("Unable to read a block at offset %lu"
						"block size %lu obj %p rc %d",
						offset+(i*bs), bs, obj, rc);
					return rc;
				}
			}
		}

		rc = 0;
	}

	log_trace("pread_aligned_handle_holes: rc=%d", rc);

	return rc;
}

static int pwrite_unaligned(struct dstore_obj *obj, off_t offset, size_t count,
			    size_t bs, char *buf, dstore_io_op_cb_t cb,
			    void *cb_ctx)
{
	int rc = 0;

	uint32_t left_blk_num = offset/bs;

	uint32_t right_blk_num = (offset + count)/bs;
	if ((offset + count) % bs == 0)
		right_blk_num--;

	uint32_t num_of_blks = right_blk_num - left_blk_num + 1;

	char *tmpBuf = calloc(num_of_blks*bs, sizeof(char));

	if (tmpBuf == NULL)
	{
		rc = -1;
		log_err("Could not allocate memory");
		goto out;
	}

	/* IO is not already left aligned, read left most block */
	if ((offset % bs) != 0)
	{
		rc = pread_aligned_handle_holes(obj, tmpBuf,
						bs, left_blk_num*bs,
						bs, cb, cb_ctx);
		if (rc < 0)
		{
			log_err("Read failed at offset %lu block size %lu ,"
				"obj %p rc %d", left_blk_num*bs, bs, obj, rc);
			goto out;
		}
	}

	/* IO is not already right aligned, read right most block */
	if ((offset + count) % bs != 0 && left_blk_num != right_blk_num)
	{
		rc = pread_aligned_handle_holes(obj, (tmpBuf+(num_of_blks-1)*bs),
						bs, right_blk_num*bs, bs,
						cb, cb_ctx);
		if (rc < 0)
		{
			log_err("Read failed at offset %lu block size %lu ,"
				"obj %p rc %d", right_blk_num*bs, bs, obj, rc);
			goto out;
		}
	}

	uint32_t buf_pos = offset - (left_blk_num * bs);
	memcpy (tmpBuf+buf_pos, buf, count);

	/* Do one write which is both left and right aligned */
	rc = pwrite_aligned(obj, tmpBuf, num_of_blks*bs,
			    left_blk_num * bs, cb, cb_ctx);

	if (rc < 0)
	{
		log_err("Write failed at offset %lu block size %lu ,"
			"obj %p rc %d", left_blk_num*bs, bs, obj, rc);
		goto out;
	}

out:

	if (tmpBuf)
	{
		free(tmpBuf);
	}

	log_trace("pwrite_unaligned: rc=%d", rc);
	return rc;
}


static int pread_unaligned(struct dstore_obj *obj, off_t offset, size_t count,
			   size_t bs, char *buf, dstore_io_op_cb_t cb,
			   void *cb_ctx)
{
	int rc = 0;
	uint32_t cont_blk_count = 0;
	uint32_t buf_pos = 0;
	uint32_t left_blk_num = 0;
	uint32_t left_bytes = 0;
	uint32_t right_bytes = 0;
	uint32_t read_count = 0;

	char *tmpBuf = calloc(bs, sizeof(char));
	if (tmpBuf == NULL)
	{
		rc = -1;
		log_err("Could not allocate memory");
		goto out;
	}

	if (offset % bs == 0 && count >= bs)
	{
		/* IO is already left aligned */
		goto continous_aligned_read;
	}

	left_blk_num = offset/bs;
	left_bytes = offset - (left_blk_num * bs);
	right_bytes = bs - left_bytes;

	/* check for an insider block */
	/* for if we have to read only 100 bytes from a block,
	 * this block is insider block */
	read_count = (count < right_bytes) ? count : right_bytes;

	/* read left most block */
	rc = pread_aligned_handle_holes(obj, tmpBuf, bs,
					left_blk_num*bs, bs, cb, cb_ctx);

	if (rc < 0)
	{
		log_err("Read failed at offset %lu block size %lu , obj %p rc %d",
			left_blk_num*bs, bs, obj, rc);
		goto out;
	}

	memcpy(buf, tmpBuf+left_bytes, read_count);

	if (count <= right_bytes)
	{
		goto out;
	}

	count = count - read_count;
	offset = offset + read_count;
	buf_pos = read_count;

continous_aligned_read:

	cont_blk_count = count/bs;

	if (cont_blk_count > 0)
	{
		rc = pread_aligned_handle_holes(obj, buf+buf_pos,
						cont_blk_count*bs, offset,
						bs, cb, cb_ctx);
		if (rc < 0)
		{
			log_err("Read failed at offset %lu block size %lu ,"
				"obj %p rc %d", offset, bs, obj, rc);
			goto out;
		}

		count = count - cont_blk_count*bs;
		offset = offset + cont_blk_count*bs;
		buf_pos = buf_pos + cont_blk_count*bs;
	}

	dassert (count >= 0);

	if (count == 0) /* Io is already right aligned */
		goto out;

	/* read the right most block */
	rc = pread_aligned_handle_holes(obj, tmpBuf, bs,
					offset, bs, cb, cb_ctx);
	if (rc < 0)
	{
		log_err("Read failed at offset %lu block size %lu ,"
			"obj %p rc %d", offset, bs, obj, rc);
		goto out;
	}

	memcpy(buf+buf_pos, tmpBuf, count);

out:
	if (tmpBuf)
		free(tmpBuf);

	log_trace("pread_unaligned: rc=%d", rc);
	return rc;
}

int dstore_io_op_pwrite(struct dstore_obj *obj, off_t offset, size_t count,
			size_t bs, char *buf)
{
	int rc = 0;

	dassert(obj);
	dassert(buf);

	if (count % bs == 0 && offset % bs == 0)
	{
		rc = pwrite_aligned(obj, buf, count, offset, NULL, NULL);
	}
	else
	{
		rc = pwrite_unaligned(obj, offset, count, bs, buf, NULL, NULL);
	}

	log_trace("dstore_io_op_pwrite: rc=%d", rc);
	return rc;
}

int dstore_io_op_pread(struct dstore_obj *obj, off_t offset, size_t count,
		       size_t bs, char *buf)
{
	int rc = 0;

	dassert(obj);
	dassert(buf);

	if (count % bs == 0 && offset % bs == 0)
	{
		rc = pread_aligned_handle_holes(obj, buf, count, offset, bs,
						NULL, NULL);
	}
	else
	{
		rc = pread_unaligned(obj, offset, count, bs, buf, NULL, NULL);
	}

	log_trace("dstore_io_op_pread: rc=%d", rc);
	return rc;
}
