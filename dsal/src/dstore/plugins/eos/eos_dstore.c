/*
 * Filename:         eos_dstore.c
 * Description:      Implementation of EOS dstore.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains APIs which implement DSAL's dstore framework,
 on top of eos clovis object APIs.
*/

#include <sys/param.h> /* DEV_BSIZE */
#include "eos/helpers.h" /* M0 wrappers from eos-utils */
#include "common/log.h" /* log_* */
#include "common/helpers.h" /* RC_WRAP* */
#include "dstore.h" /* import public DSTORE API definitions */
#include "../../dstore_internal.h" /* import private DSTORE API definitions */
/* TODO: assert() calls should be replaced gradually with dassert() calls. */
#include <assert.h> /* assert() */
#include "debug.h" /* dassert */
#include "lib/vec.h" /* m0bufvec and m0indexvec */

/** Private definition of DSTORE object for M0-based backend. */
struct eos_dstore_obj {
	struct dstore_obj base;
	struct m0_clovis_obj cobj;
};
_Static_assert((&((struct eos_dstore_obj *) NULL)->base) == 0,
	       "The offset of of the base field should be zero.\
	       Otherwise, the direct casts (E2D, D2E) will not work.");

/* Casts DSTORE Object to EOS Object.
 *
 * Note: Due to the lack of de-referencing, this call may be safely
 * used before pre-conditions, e.g.:
 * @code
 *   struct eos_dstore_obj *obj = D2E_obj(in);
 *   assert(obj);
 * @endcode
 * @see ::E2D_obj
 */
static inline
struct eos_dstore_obj *D2E_obj(struct dstore_obj *obj)
{
	return (struct eos_dstore_obj *) obj;
}

/* Casts EOS Object to DSTORE Object.
 * @see ::D2E_obj
 */
static inline
struct dstore_obj *E2D_obj(struct eos_dstore_obj *obj)
{
	return (struct dstore_obj *) obj;
}


/** IO Buffers and extents for M0-based backend.
 * This object holds the information associated with an IO operation:
 *	- IO buffers - array of sizes, array of pointers, and count.
 *	- IO range (or extents) - array of sizes,
 *		array of offsets (in the object), and count.
 * This object is needed to keep M0-related information in the one place.
 * Memory mgmt: the object keeps only references borrowed from
 * the base IO operation object (dstore_io_op.)
 */
struct eos_io_bufext {
	/** Vector of data buffers. */
	struct m0_bufvec data;
	/* Vector of extents in the target object */
	struct m0_indexvec extents;
};

/** Private definition of DSTORE io operation for M0-based backend.
 * Memory mgmt:
 * 1. It is a self-referential structure. Additional precautions should be
 *	taken care of when this structure needs to be copied or moved.
 *	Since m0 op holds the pointer to the operation context, which in our
 *	case is the eos_io_op, this structure cannot be moved without
 *	"re-wiring" of the op_datum field inside `cop`.
 * 2. The base field of this structure holds borrowed references to the
 *	user-provided buffers. It does not own any data, and therefore
 *	it should not free any user data. However, it may own additional
 *	resources, for example a vector of sizes or offsets.
 * 3. The vec field of this structure hold copies of the references from the
 *	base field. It is needed because the requirements state that DSAL
 *	should have a common base type for operation and data vectors,
 *	however it is not possible for M0-dependent data types (bufvec
 *	and indexvec) to be "common" data types.
 *
 * Here is an example of values for a case where IO operation should be
 * executed for two IO buffers and two different offsets:
 *
 * @{verbatim}
 * +-------------------------------------------------------------------------+
 * | 0xCAFE, 0xD0AB is the addresses of two buffers allocated		     |
 * | by the user (for example, after a malloc() call).                       |
 * |                                                                         |
 * | .base.data.dbufs  => {{ 0xCAFE }, { 0xD0AB }}                           |
 * |		points to an array of user-provided buffers where            |
 * |		the array itself is allocated by DSAL.                       |
 * | .base.data.ovec => { 0, 4096 }					     |
 * |		points to an array of offsets allocated by DSAL.             |
 * | .base.data.svec => { 4096, 4096 }					     |
 * |		points to an array of sizes allocated by DSAL.               |
 * | .vec.data.ov_buf == .base.data.dbufs                                    |
 * |		points to the same location as the base field.               |
 * | .vec.extents.iv_buf == .base.data.ovec                                  |
 * |		points to the same location as the base field.               |
 * +-------------------------------------------------------------------------+
 * @{endverbatim}
 *
 * As per the current state of M0 API, "attrs" does not have any semantic
 * meaning for us (the M0 API users), so that they are kept zeroed (empty).
 *
 * Further improvements for this data type.
 *	1. Re-usage of IO operation objects. M0 operation can be re-used,
 *	therefore it would be good to make eos_io_op to be re-usable as well.
 *	This improvement will be when the users start maintain
 *	operation lists.
 */
struct eos_io_op {
	struct dstore_io_op base;
	struct m0_clovis_op *cop;
	struct eos_io_bufext vec;
	struct m0_bufvec attrs;
};

_Static_assert((&((struct eos_io_op *) NULL)->base) == 0,
	       "The offset of of the base field should be zero.\
	       Otherwise, the direct casts (E2D, D2E) will not work.");

static inline
struct eos_io_op *D2E_op(struct dstore_io_op *op)
{
	return (struct eos_io_op *) op;
}

/* Casts EOS IO operation to DSTORE IO Operation
 */
static inline
struct dstore_io_op *E2D_op(struct eos_io_op *op)
{
	return (struct dstore_io_op *) op;
}

enum update_stat_type {
	UP_ST_WRITE = 1,
	UP_ST_READ = 2,
	UP_ST_TRUNCATE = 3
};

static int update_stat(struct stat *stat, enum update_stat_type utype,
		       off_t size)
{
	struct timeval t;

	if (!stat) {
		return -EINVAL;
	}

	if (gettimeofday(&t, NULL) != 0) {
		return -errno;
	}

	switch (utype) {
	case UP_ST_WRITE:
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
		stat->st_ctim = stat->st_mtim;
		if (size > stat->st_size) {
			stat->st_size = size;
			stat->st_blocks = (size + DEV_BSIZE - 1) / DEV_BSIZE;
		}
		break;

	case UP_ST_READ:
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
		break;

	case UP_ST_TRUNCATE:
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
		stat->st_mtim = stat->st_ctim;
		stat->st_size = size;
		stat->st_blocks = size / DEV_BSIZE;
		break;

	default: /* Should not occur */
		return -EINVAL;
		break;
	}

	return 0;
}

int eos_ds_obj_get_id(struct dstore *dstore, dstore_oid_t *oid)
{
	return m0_ufid_get((struct m0_uint128 *)oid);
}

int eos_ds_obj_create(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid)
{
	int rc;
	struct m0_uint128 fid;

	assert(oid != NULL);
	m0_fid_copy((struct m0_uint128 *)oid, &fid);

	RC_WRAP_LABEL(rc, out, m0store_create_object, fid);

out:
	log_debug("ctx=%p fid = "U128X_F" rc=%d", ctx, U128_P(&fid), rc);
	return rc;
}

int eos_ds_init(struct collection_item *cfg_items)
{
	return m0init(cfg_items);
}

int eos_ds_fini(void)
{
	m0fini();
	return 0;
}

int eos_ds_obj_del(struct dstore *dstore, void *ctx, dstore_oid_t *oid)
{
	struct m0_uint128 fid;
	int rc;

	assert(oid != NULL);

	m0_fid_copy((struct m0_uint128 *)oid, &fid);

	/* Delete the object from backend store */
	rc = m0store_delete_object(fid);
	if (rc) {
		if (rc == -ENOENT) {
			log_warn("Non-existing obj, ctx=%p fid= "U128X_F" rc=%d",
				  ctx, U128_P(&fid), rc);
			goto out;
		}
		log_err("Unable to delete object, ctx=%p fid= "U128X_F" rc=%d",
			 ctx, U128_P(&fid), rc);
		goto out;
	}

out:
	log_debug("EXIT: ctx=%p fid= "U128X_F" rc=%d", ctx, U128_P(&fid), rc);
	return rc;
}

int eos_ds_obj_read(struct dstore *dstore, void *ctx, dstore_oid_t *oid,
	 	    off_t offset, size_t buffer_size, void *buffer,
		    bool *end_of_file, struct stat *stat)
{
	int rc;
	ssize_t read_bytes;
	ssize_t bsize;
	struct m0_uint128 fid;

	m0_fid_copy((struct m0_uint128 *)oid, &fid);

	bsize = m0store_get_bsize(fid);
	if (bsize < 0) {
		rc = bsize;
		goto out;
	}

	read_bytes = m0store_pread(fid, offset, buffer_size,
				   bsize, buffer);
	if (read_bytes < 0) {
		rc = read_bytes;
		goto out;
	}

	RC_WRAP_LABEL(rc, out, update_stat, stat, UP_ST_READ, 0);
	rc = read_bytes;
out:
	log_debug("oid=%" PRIx64 ":%" PRIx64 " read_bytes=%lu",
		  oid->f_hi, oid->f_lo, read_bytes);
	return rc;
}

int eos_ds_obj_write(struct dstore *dstore, void *ctx, dstore_oid_t *oid,
		     off_t offset, size_t buffer_size,
		     void *buffer, bool *fsal_stable, struct stat *stat)
{
	int rc;
	ssize_t written_bytes;
	struct m0_uint128 fid;
	ssize_t bsize;

	m0_fid_copy((struct m0_uint128 *)oid, &fid);

	bsize = m0store_get_bsize(fid);
	if (bsize < 0) {
		rc = bsize;
		goto out;
	}

	written_bytes = m0store_pwrite(fid, offset, buffer_size,
				       bsize, buffer);
	if (written_bytes < 0) {
		rc = written_bytes;
		goto out;
	}

	RC_WRAP_LABEL(rc, out, update_stat, stat, UP_ST_WRITE,
		offset + written_bytes);
	rc = written_bytes;
	*fsal_stable = true;
out:
	log_debug("oid=%" PRIx64 ":%" PRIx64 " written_bytes=%lu",
		  oid->f_hi, oid->f_lo, written_bytes);
	return rc;
}

int eos_ds_obj_resize(struct dstore *dstore, void *ctx,
		       dstore_oid_t *oid,
		       size_t old_size,
		       size_t new_size)
{
	int rc = 0;
	off_t offset;
	size_t count;
	struct m0_uint128 fid;

	assert(ctx && oid);

	if (old_size == new_size) {
		log_debug("new size == old size == %llu",
			  (unsigned long long) old_size);
		rc = 0;
		goto out;
	}

	if (old_size < new_size) {
		log_debug("punching a hole in the file: %llu -> %llu",
			  (unsigned long long) old_size,
			  (unsigned long long) new_size);
		rc = 0;
		goto out;
	}

	assert(old_size > new_size);

	log_debug("TRUNC: oid=%" PRIx64 ":%" PRIx64 ", size %llu -> %llu",
		  oid->f_hi, oid->f_lo,
		  (unsigned long long) old_size,
		  (unsigned long long) new_size);

	count = old_size - new_size;
	offset = new_size;
	m0_fid_copy((struct m0_uint128 *)oid, &fid);

	rc = m0_file_unmap(fid, count, offset);
	if (rc != 0) {
		log_err("Failed to unmap count=%llu, offset=%llu",
			(unsigned long long) count,
			(unsigned long long) offset);
	}
out:
	return rc;
}

static int eos_dstore_obj_alloc(struct eos_dstore_obj **out)
{
	int rc = 0;
	struct eos_dstore_obj *obj;

	M0_ALLOC_PTR(obj);
	if (!obj) {
		rc = RC_WRAP_SET(-ENOMEM);
		goto out;
	}

	*out = obj;

out:
	return rc;
}

static void eos_dstore_obj_free(struct eos_dstore_obj *obj)
{
	m0_free(obj);
}

static int eos_ds_obj_open(struct dstore *dstore, const obj_id_t *oid,
			   struct dstore_obj **out)
{
	int rc;
	struct eos_dstore_obj *obj = NULL;

	RC_WRAP_LABEL(rc, out, eos_dstore_obj_alloc, &obj);

	RC_WRAP_LABEL(rc, out, m0store_obj_open, oid, &obj->cobj);

	*out = E2D_obj(obj);
	obj = NULL;

out:
	eos_dstore_obj_free(obj);
	return rc;
}

static int eos_ds_obj_close(struct dstore_obj *dobj)
{
	struct eos_dstore_obj *obj = D2E_obj(dobj);

	dassert(obj);
	dassert(obj->base.ds);

	m0store_obj_close(&obj->cobj);
	eos_dstore_obj_free(obj);

	/* XXX:
	 * Right now, we assume that M0-based backend
	 * cannot fail here. However, later on
	 * we may want to return an error if we have unfinished
	 * M0 operations. Such operations (that have not been
	 * properly finished) may cause failures, for instance,
	 * during finalization of DSAL module.
	 */
	return 0;
}

/* NOTE: The function may be unsafe because we have two references to
 * the same object (svec) from two different objects (data and extents).
 * However, it is safe as long as M0 is not trying to modify both of them
 * as independent objects (for example, using memcpy or something similar).
 * Since logically M0 should not mutate these fields (READ,WRITE,ALLOC,FREE
 * do not modify the vectors except the contents of data buffers), it is
 * safe to do this assignment here.
 */
static inline
void dstore_io_vec2bufext(struct dstore_io_vec *io_vec,
			  struct eos_io_bufext *bufext)
{
	M0_SET0(bufext);

	bufext->data.ov_buf = (void **) io_vec->dbufs;
	bufext->data.ov_vec.v_nr = io_vec->nr;
	bufext->data.ov_vec.v_count = io_vec->svec;

	bufext->extents.iv_vec.v_nr = io_vec->nr;
	bufext->extents.iv_vec.v_count = io_vec->svec;
	bufext->extents.iv_index = io_vec->ovec;
}

static void on_oop_executed(struct m0_clovis_op *cop)
{
	log_trace("IO op %p executed.", cop->op_datum);
	/* noop */
}


static void on_oop_finished(struct m0_clovis_op *cop)
{
	int rc = m0_clovis_rc(cop);
	struct eos_io_op *op = cop->op_datum;
	dassert(op->cop == cop);
	RC_WRAP_SET(rc);
	if (op->base.cb) {
		op->base.cb(op->base.cb_ctx, &op->base, rc);
	}
	log_trace("IO op %p finished.", op);
}

static void on_oop_failed(struct m0_clovis_op *cop)
{
	log_trace("IO op %p went to failed state.", cop->op_datum);
	on_oop_finished(cop);
}

static const struct m0_clovis_op_ops eos_io_op_cbs = {
	.oop_executed = on_oop_executed,
	.oop_failed = on_oop_failed,
	.oop_stable = on_oop_finished,
};

static int eos_ds_io_op_init(struct dstore_obj *dobj,
			     enum dstore_io_op_type type,
			     struct dstore_io_vec *bvec,
			     dstore_io_op_cb_t cb,
			     void *cb_ctx,
			     struct dstore_io_op **out)
{
	int rc = 0;
	struct eos_dstore_obj *obj = D2E_obj(dobj);
	struct eos_io_op *result = NULL;

	const m0_time_t schedule_now = 0;
	const uint64_t empty_mask = 0;

	if (!M0_IN(type, (DSTORE_IO_OP_WRITE))) {
		log_err("%s", (char *) "Unsupported IO operation");
		rc = RC_WRAP_SET(-EINVAL);
		goto out;
	}

	dassert(bvec);
	dassert(out);
	dassert(dstore_io_vec_invariant(bvec));

	M0_ALLOC_PTR(result);
	if (result == NULL) {
		rc = RC_WRAP_SET(-ENOMEM);
		goto out;
	}

	result->base.type = type;
	result->base.obj = dobj;

	dstore_io_vec_move(&result->base.data, bvec);

	dstore_io_vec2bufext(&result->base.data, &result->vec);

	RC_WRAP_LABEL(rc, out, m0_clovis_obj_op, &obj->cobj, M0_CLOVIS_OC_WRITE,
		      &result->vec.extents, &result->vec.data,
		      &result->attrs, empty_mask, &result->cop);

	result->cop->op_datum = result;
	m0_clovis_op_setup(result->cop, &eos_io_op_cbs, schedule_now);

	*out = E2D_op(result);
	result = NULL;

out:
	if (result) {
		m0_free(result);
	}

	log_debug("io_op_init obj=%p, nr=%d, op=%p rc=%d", obj, (int) bvec->nr,
		  rc == 0 ? *out : NULL, rc);

	dassert((!(*out)) || dstore_io_op_invariant(*out));
	return rc;
}

static int eos_ds_io_op_submit(struct dstore_io_op *dop)
{
	struct eos_io_op *op = D2E_op(dop);
	m0_clovis_op_launch(&op->cop, 1);
	log_debug("io_op_submit op=%p", op);
	return 0; /* M0 launch is safe */
}

static int eos_ds_io_op_wait(struct dstore_io_op *dop)
{
	int rc;
	struct eos_io_op *op = D2E_op(dop);
	const uint64_t wait_bits = M0_BITS(M0_CLOVIS_OS_FAILED,
					   M0_CLOVIS_OS_STABLE);
	const m0_time_t time_limit = M0_TIME_NEVER;

	RC_WRAP_LABEL(rc, out, m0_clovis_op_wait, op->cop, wait_bits,
		      time_limit);
	RC_WRAP_LABEL(rc, out, m0_clovis_rc, op->cop);

out:
	log_debug("io_op_wait op=%p, rc=%d", op, rc);
	return rc;
}

static void eos_ds_io_op_fini(struct dstore_io_op *dop)
{
	struct eos_io_op *op = D2E_op(dop);

	m0_clovis_op_fini(op->cop);
	m0_clovis_op_free(op->cop);
	m0_free(op);
}

const struct dstore_ops eos_dstore_ops = {
	.init = eos_ds_init,
	.fini = eos_ds_fini,
	.obj_create = eos_ds_obj_create,
	.obj_delete = eos_ds_obj_del,
	.obj_read = eos_ds_obj_read,
	.obj_write = eos_ds_obj_write,
	.obj_resize = eos_ds_obj_resize,
	.obj_get_id = eos_ds_obj_get_id,
	.obj_open = eos_ds_obj_open,
	.obj_close = eos_ds_obj_close,
	.io_op_init = eos_ds_io_op_init,
	.io_op_submit = eos_ds_io_op_submit,
	.io_op_wait = eos_ds_io_op_wait,
	.io_op_fini = eos_ds_io_op_fini,
};
