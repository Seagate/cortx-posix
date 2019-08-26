/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* extstore.c
 * KVSNS: implement a dummy object store inside a POSIX directory
 */

#include <ini_config.h>
#include <kvsns/extstore.h>
#include "../../common/mero/m0common.h"
#include "kvsns/log.h"

static int build_m0store_id(kvsns_ino_t	 object,
			    struct m0_uint128  *id)
{
	char k[KLEN];
	char v[VLEN];
	size_t klen, vlen;
	int rc;

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.data", object);
	klen = rc;

	rc = m0kvs_get(k, klen, v, &vlen);
	if (rc != 0)
		goto out;

	*id = M0_CLOVIS_ID_APP;
	id->u_lo = atoi(v);
out:
	return rc;
}

enum update_stat_how {
	UP_ST_WRITE = 1,
	UP_ST_READ = 2,
	UP_ST_TRUNCATE = 3
};

static int update_stat(struct stat *stat, enum update_stat_how how,
		       off_t size)
{
	struct timeval t;

	if (!stat)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	switch (how) {
	case UP_ST_WRITE:
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
		stat->st_ctim = stat->st_mtim;
		if (size > stat->st_size) {
			stat->st_size = size;
			stat->st_blocks = size / DEV_BSIZE;
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

int extstore_create(kvsns_ino_t object)
{
	char k[KLEN];
	char v[VLEN];
	size_t klen;
	size_t vlen;
	int rc;
	struct m0_uint128 id;

	snprintf(k, KLEN, "%llu.data", object);
	klen = strnlen(k, KLEN) + 1;

	id = M0_CLOVIS_ID_APP;
	id.u_lo += (unsigned long long)object;
	snprintf(v, VLEN, "%llu", (unsigned long long)id.u_lo);
	vlen = strnlen(v, KLEN) + 1;

	rc = m0kvs_set(k, klen, v, vlen);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.data_ext", object);
	klen = strnlen(k, KLEN) + 1;
	snprintf(v, VLEN, " ");
	vlen = strnlen(v, KLEN) + 1;

	rc = m0kvs_set(k, klen, v, vlen);
	if (rc != 0)
		return rc;

	rc = m0store_create_object(id);
	if (rc != 0)
		return rc;

	return 0;
}

int extstore_get_new_kfid(kvsns_ino_t ino, kvsns_fid_t *kfid)
{
	return m0_ufid_get((struct m0_uint128 *)kfid);
}

int extstore_ino_to_fid(void *ctx, kvsns_ino_t object, kvsns_fid_t *kfid)
{
	char k[KLEN];
	char v[VLEN];
	size_t vlen = VLEN;
	size_t klen;
	int rc;

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.data", object);
	klen = rc;
	rc = m0kvs2_get(ctx, k, klen, v, &vlen);
	if (rc != 0) {
		log_err("Unable to get FID for inode=%llu rc=%d",
			object, rc);
		goto out;
	}

	RC_WRAP_LABEL(rc, out, m0_fid_sscanf, v, (struct m0_fid *)kfid);

out:
	log_debug("rc=%d inode=%llu FID=%s", rc, object, v);
	return rc;
}

int extstore2_create(void *ctx, kvsns_fid_t *kfid)
{
	int rc;
	struct m0_uint128 fid;

	KVSNS_DASSERT(kfid != NULL);
	m0_fid_copy((struct m0_uint128 *)kfid, &fid);

	RC_WRAP_LABEL(rc, out, m0store_create_object, fid);
	if (rc != 0)
		goto out;

out:
	log_debug("ctx=%p fid = "U128X_F" rc=%d", ctx, U128_P(&fid), rc);
	return rc;
}

int extstore_attach(kvsns_ino_t *ino, char *objid, int objid_len)
{
	char k[KLEN];
	char v[VLEN];
	size_t klen;
	size_t vlen;
	int rc;
	struct m0_uint128 id;


	snprintf(k, KLEN, "%llu.data", *ino);
	klen = strnlen(k, KLEN) + 1;
	id = M0_CLOVIS_ID_APP;
	id.u_lo = atoi(objid);

	snprintf(v, VLEN, "%llu", (unsigned long long)id.u_lo);
	vlen = strnlen(v, KLEN) + 1;

	rc = m0kvs_set(k, klen, v, vlen);
	if (rc != 0)
		return rc;

	snprintf(k, KLEN, "%llu.data", *ino);
	snprintf(v, VLEN, " ");
	vlen = strnlen(v, KLEN) + 1;

	rc = m0kvs_set(k, klen, v, vlen);
	if (rc != 0)
		return rc;

	rc = m0store_create_object(id);
	if (rc != 0)
		return rc;

	return 0;
}

int extstore_init(struct collection_item *cfg_items)
{
	/* Init m0store */
	return m0init(cfg_items);
//	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	char k[KLEN];
	size_t klen;
	struct m0_uint128 id;
	int rc;

	rc = build_m0store_id(*ino, &id);
	if (rc) {
		if (rc == -ENOENT) /* No data created */
			return 0;

		return rc;
	}

	rc = m0store_delete_object(id);
	if (rc) {
		if (errno == ENOENT)
			return 0;

		return -errno;
	}

	/* delete <inode>.data */
	snprintf(k, KLEN, "%llu.data", *ino);
	klen = strnlen(k, KLEN) + 1;
	rc = m0kvs_del(k, klen);
	if (rc != 0)
		return rc;

	/* delete <inode>.data_ext */
	snprintf(k, KLEN, "%llu.data_ext", *ino);
	klen = strnlen(k, KLEN) + 1;
	rc = m0kvs_del(k, klen);
	if (rc != 0)
		return rc;

	return 0;
}

int extstore2_del(void *ctx, kvsns_fid_t *kfid)
{
	struct m0_uint128 fid;
	int rc;

	KVSNS_DASSERT(kfid != NULL);

	m0_fid_copy((struct m0_uint128 *)kfid, &fid);

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

int extstore_read(kvsns_ino_t *ino,
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  bool *end_of_file,
		  struct stat *stat)
{
	ssize_t read_bytes;
	ssize_t bsize;
	struct m0_uint128 id;

	RC_WRAP(build_m0store_id, *ino, &id);

	bsize = m0store_get_bsize(id);
	if (bsize < 0)
		return (int)bsize;

	read_bytes = m0store_pread(id, offset, buffer_size,
				   bsize, buffer);
	if (read_bytes < 0)
		return -1;

	RC_WRAP(update_stat, stat, UP_ST_READ, 0);
	return read_bytes;
}


int extstore2_read(void *ctx, kvsns_fid_t *kfid, off_t offset,
		   size_t buffer_size, void *buffer, bool *end_of_file,
		   struct stat *stat)
{
	int rc;
	ssize_t read_bytes;
	ssize_t bsize;
	struct m0_uint128 fid;

	m0_fid_copy((struct m0_uint128 *)kfid, &fid);

	bsize = m0store_get_bsize(fid);
	if (bsize < 0) {
		rc = bsize;
		goto out;
	}

	read_bytes = m0store_pread(fid, offset, buffer_size,
				   bsize, buffer);
	if (read_bytes < 0) {
		rc = -1;
		goto out;
	}

	RC_WRAP(update_stat, stat, UP_ST_READ, 0);
	rc = read_bytes;
out:
	log_debug("fid=%" PRIx64 ":%" PRIx64 " read_bytes=%lu",
		  kfid->f_hi, kfid->f_lo, read_bytes);
	return rc;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   bool *fsal_stable,
		   struct stat *stat)
{
	ssize_t written_bytes;
	struct m0_uint128 id;
	ssize_t bsize;

	RC_WRAP(build_m0store_id, *ino, &id);

	bsize = m0store_get_bsize(id);
	if (bsize < 0)
		return (int)bsize;

	written_bytes = m0store_pwrite(id, offset, buffer_size,
				       bsize, buffer);
	if (written_bytes < 0)
		return -1;

	RC_WRAP(update_stat, stat, UP_ST_WRITE,
		offset+written_bytes);

	*fsal_stable = true;
	return written_bytes;
}

int extstore2_write(void *ctx, kvsns_fid_t *kfid, off_t offset, size_t buffer_size,
		    void *buffer, bool *fsal_stable, struct stat *stat)
{
	int rc;
	ssize_t written_bytes;
	struct m0_uint128 fid;
	ssize_t bsize;

	m0_fid_copy((struct m0_uint128 *)kfid, &fid);

	bsize = m0store_get_bsize(fid);
	if (bsize < 0) {
		rc = bsize;
		goto out;
	}

	written_bytes = m0store_pwrite(fid, offset, buffer_size,
				       bsize, buffer);
	if (written_bytes < 0) {
		rc = -1;
		goto out;
	}

	RC_WRAP(update_stat, stat, UP_ST_WRITE,
		offset + written_bytes);
	rc = written_bytes;
	*fsal_stable = true;
out:
	log_debug("fid=%" PRIx64 ":%" PRIx64 " written_bytes=%lu",
		  kfid->f_hi, kfid->f_lo, written_bytes);
	return rc;
}

/* TODO:DEPRECATED: Can be safely removed when the caller code is removed. */
int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize,
		      bool on_obj_store,
		      struct stat *stat)
{
	if (!ino || !stat)
		return -EINVAL;

	stat->st_size = filesize;

	return 0;
}


int extstore2_truncate(void *ctx,
		       kvsns_fid_t *kfid,
		       size_t old_size,
		       size_t new_size)
{
	int rc = 0;
	off_t offset;
	size_t count;
	struct m0_uint128 fid;

	KVSNS_DASSERT(ctx && kfid);

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

	KVSNS_DASSERT(old_size > new_size);

	log_debug("TRUNC: fid=%" PRIx64 ":%" PRIx64 ", size %llu -> %llu",
		  kfid->f_hi, kfid->f_lo,
		  (unsigned long long) old_size,
		  (unsigned long long) new_size);

	count = old_size - new_size;
	offset = new_size;
	m0_fid_copy((struct m0_uint128 *)kfid, &fid);

	rc = m0_file_unmap(fid, count, offset);
	if (rc != 0) {
		log_err("Failed to unmap count=%llu, offset=%llu",
			(unsigned long long) count,
			(unsigned long long) offset);
	}
out:
	return rc;
}
