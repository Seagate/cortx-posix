/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) Panasas Inc., 2011
 * Author: Jim Lieb jlieb@panasas.com
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * -------------
 */

/* file.c
 * File I/O methods for KVSFS module
 */

/* These functions either non-implemented, deprecated or moved into
 * handle.c module.
 * TODO: This file must be deleted when IO opearartions are implemented.
 */

#if 0
#include "config.h"

#include <assert.h>
#include "fsal.h"
#include "fsal_internal.h"
#include "FSAL/access_check.h"
#include "fsal_convert.h"
#include <unistd.h>
#include <fcntl.h>
#include "FSAL/fsal_commonlib.h"
#include "kvsfs_methods.h"
#include <stdbool.h>

/* create
 * create a regular file and set its attributes
 */

static fsal_status_t kvsfs_create(struct fsal_obj_handle *dir_hdl,
				 const char *name, struct attrlist *attrib,
				 struct fsal_obj_handle **handle)
{
	struct kvsfs_fsal_obj_handle *myself, *hdl;
	int retval = 0;
	efs_cred_t cred;
	efs_ino_t object;
	struct stat stat;
	efs_fs_ctx_t fs_ctx = EFS_NULL_FS_CTX;

	*handle = NULL;		/* poison it */
	if (!fsal_obj_handle_is(dir_hdl, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"Parent handle is not a directory. hdl = 0x%p",
			dir_hdl);
		return fsalstat(ERR_FSAL_NOTDIR, 0);
	}
	myself = container_of(dir_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);

	cred.uid = attrib->owner;
	cred.gid = attrib->group;

	retval = kvsfs_obj_to_cortxfs_ctx(dir_hdl, &fs_ctx);
	if (retval != 0) {
		LogCrit(COMPONENT_FSAL, "Unable to get fs_handle: %d", retval);
		goto fileerr;
	}

	retval = cortxfs2_creat(fs_ctx, &cred, &myself->handle->kvsfs_handle,
			        (char *)name, fsal2unix_mode(attrib->mode), &object);
	if (retval)
		goto fileerr;

	retval = cortxfs2_getattr(fs_ctx, &cred, &object, &stat);
	if (retval)
		goto fileerr;

	construct_handle(op_ctx->fsal_export, &object, &stat, handle);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

 fileerr:
	return fsalstat(posix2fsal_error(-retval), -retval);
}
/** kvsfs_open
 * called with appropriate locks taken at the cache inode level
 */

fsal_status_t kvsfs_open(struct fsal_obj_handle *obj_hdl,
			fsal_openflags_t openflags)
{
	struct kvsfs_fsal_obj_handle *myself;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int rc = 0;
	efs_cred_t cred;
	cortxfs_fs_ctx_t fs_ctx = CORTXFS_NULL_FS_CTX;

	cred.uid = op_ctx->creds->caller_uid;
	cred.gid = op_ctx->creds->caller_gid;

	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle, obj_handle);

	assert(myself->u.file.openflags == FSAL_O_CLOSED);

	rc = kvsfs_obj_to_cortxfs_ctx(obj_hdl, &fs_ctx);
	if (rc) {
		LogCrit(COMPONENT_FSAL, "Unable to get fs_handle: %d", rc);
		goto errout;
	}

	rc = cortxfs2_open(fs_ctx, &cred, &myself->handle->kvsfs_handle, O_RDWR,
			   0777, &myself->u.file.fd);

	if (rc)
		goto errout;

	/* >> fill output struct << */
	myself->u.file.openflags = openflags;

	/* save the stat */
	rc = cortxfs2_getattr(fs_ctx, &cred, &myself->handle->kvsfs_handle,
			      &myself->u.file.saved_stat);

errout:
	if (rc) {
		fsal_error = posix2fsal_error(-rc);
		return fsalstat(fsal_error, -rc);
	}

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* kvsfs_status
 * Let the caller peek into the file's open/close state.
 */

fsal_openflags_t kvsfs_status(struct fsal_obj_handle *obj_hdl)
{
	struct kvsfs_fsal_obj_handle *myself;

	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle,
			      obj_handle);
	return myself->u.file.openflags;
}

/* kvsfs_read
 * concurrency (locks) is managed in cache_inode_*
 */

fsal_status_t kvsfs_read(struct fsal_obj_handle *obj_hdl,
			uint64_t offset,
			size_t buffer_size, void *buffer, size_t *read_amount,
			bool *end_of_file)
{
	struct kvsfs_fsal_obj_handle *myself;
	cortxfs_fs_ctx_t fs_ctx = CORTXFS_NULL_FS_CTX;
	int retval = 0;
	efs_cred_t cred;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;

	cred.uid = op_ctx->creds->caller_uid;
	cred.gid = op_ctx->creds->caller_gid;

	retval = kvsfs_obj_to_cortxfs_ctx(obj_hdl, &fs_ctx);
	if (retval) {
		LogCrit(COMPONENT_FSAL, "Unable to get fs_handle: %d", retval);
		goto errout;
	}
	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle, obj_handle);

	assert(myself->u.file.openflags != FSAL_O_CLOSED);

	retval = cortxfs2_read(fs_ctx, &cred, &myself->u.file.fd,
		   	       buffer, buffer_size, offset);


	/* With FSAL_ZFS, "end of file" is always returned via a last call,
	 * once every data is read. The result is a last,
	 * empty call which set end_of_file to true */
	if (retval < 0) {
		goto errout;
	} else if (retval == 0) {
		*end_of_file = true;
		*read_amount = 0;
	} else {
		*end_of_file = false;
		*read_amount = retval;
	}

errout:
	if (retval < 0) {
		fsal_error = posix2fsal_error(-retval);
		return fsalstat(fsal_error, -retval);
	}

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* kvsfs_write
 * concurrency (locks) is managed in cache_inode_*
 */

fsal_status_t kvsfs_write(struct fsal_obj_handle *obj_hdl,
			 uint64_t offset,
			 size_t buffer_size, void *buffer,
			 size_t *write_amount, bool *fsal_stable)
{
	struct kvsfs_fsal_obj_handle *myself;
	cortxfs_fs_ctx_t fs_ctx = CORTXFS_NULL_FS_CTX;
	efs_cred_t cred;
	int retval = 0;

	cred.uid = op_ctx->creds->caller_uid;
	cred.gid = op_ctx->creds->caller_gid;

	retval = kvsfs_obj_to_cortxfs_ctx(obj_hdl, &fs_ctx);
	if (retval) {
		LogCrit(COMPONENT_FSAL, "Unable to get fs_handle: %d", retval);
		goto errout;
	}

	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle, obj_handle);

	assert(myself->u.file.openflags != FSAL_O_CLOSED);

	retval = cortxfs2_write(fs_ctx, &cred, &myself->u.file.fd,
			        buffer, buffer_size, offset);
	if (retval < 0)
		goto errout;
	*write_amount = retval;
	*fsal_stable = false;

errout:
	if (retval < 0) {
		return fsalstat(posix2fsal_error(-retval), -retval);
	}

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}


/* kvsfs_close
 * Close the file if it is still open.
 * Yes, we ignor lock status.  Closing a file in POSIX
 * releases all locks but that is state and cache inode's problem.
 */

fsal_status_t kvsfs_close(struct fsal_obj_handle *obj_hdl)
{
	struct kvsfs_fsal_obj_handle *myself;
	cortxfs_fs_ctx_t fs_ctx = CORTXFS_NULL_FS_CTX;
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;

	assert(obj_hdl->type == REGULAR_FILE);
	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle, obj_handle);

	retval = kvsfs_obj_to_cortxfs_ctx(obj_hdl, &fs_ctx);
	if (retval) {
		fsal_error = posix2fsal_error(-retval);
		LogCrit(COMPONENT_FSAL, "Unable to get fs_handle: %d", retval);
		goto errout;
	}

	if (myself->u.file.openflags != FSAL_O_CLOSED) {
		retval = cortxfs2_close(fs_ctx, &myself->u.file.fd);
		if (retval < 0)
			fsal_error = posix2fsal_error(-retval);

		myself->u.file.openflags = FSAL_O_CLOSED;
	}

errout:
	return fsalstat(fsal_error, -retval);
}

/* kvsfs_lru_cleanup
 * free non-essential resources at the request of cache inode's
 * LRU processing identifying this handle as stale enough for resource
 * trimming.
 */

fsal_status_t kvsfs_lru_cleanup(struct fsal_obj_handle *obj_hdl,
			       lru_actions_t requests)
{
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

fsal_status_t kvsfs_lock_op(struct fsal_obj_handle *obj_hdl,
			   void *p_owner,
			   fsal_lock_op_t lock_op,
			   fsal_lock_param_t *request_lock,
			   fsal_lock_param_t *conflicting_lock)
{
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

#endif
