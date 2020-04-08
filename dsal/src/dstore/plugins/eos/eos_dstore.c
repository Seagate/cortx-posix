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

#include <ini_config.h>
#include <eos/helpers.h>
#include <sys/param.h>
#include <common/log.h>
#include <common/helpers.h>
#include "dstore.h"
#include "../../dstore_internal.h"
#include <assert.h>


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
			rc = 0;
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

const struct dstore_ops eos_dstore_ops = {
	.init = eos_ds_init,
	.fini = eos_ds_fini,
	.obj_create = eos_ds_obj_create,
	.obj_delete = eos_ds_obj_del,
	.obj_read = eos_ds_obj_read,
	.obj_write = eos_ds_obj_write,
	.obj_resize = eos_ds_obj_resize,
	.obj_get_id = eos_ds_obj_get_id,
};
