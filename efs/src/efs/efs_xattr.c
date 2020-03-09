/*
 * Filename:         efs_xattr.c
 * Description:      EOS file system XATTR interfaces

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system XATTR interfaces.
*/

#include <common/log.h> /* log_*() */
#include <common/helpers.h> /* RC_LABEL*() */
#include <efs.h> /* efs_* */
#include <dstore.h> /* dstore_oid_t */
#include <md_xattr.h> /* md_xattr_exists */
#include "efs_internal.h" /*efs_ino_to_oid */
#include <errno.h> /* ERANGE */
#include <string.h> /* memcpy */
#include <sys/xattr.h> /* XATTR_CREATE */

int efs_setxattr(efs_ctx_t fs_ctx, const efs_cred_t *cred,
		 const efs_ino_t *ino, const char *name, char *value,
		 size_t size, int flags)
{
	int rc;
	dstore_oid_t oid;
	bool xattr_exists;

	dassert(cred && ino && name && value);

	if ((flags != XATTR_CREATE) && (flags != XATTR_REPLACE) &&
	   (flags != 0)) {
		rc = -EINVAL;
		goto out;
	}

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, ino,
		      EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, fs_ctx, ino, &oid);

	if ((flags == XATTR_CREATE) || (flags == XATTR_REPLACE)) {
		RC_WRAP_LABEL(rc, out, md_xattr_exists, fs_ctx,
			     (obj_id_t *)&oid, name, &xattr_exists);

		if (flags ==  XATTR_CREATE && xattr_exists) {
			rc  = -EEXIST;
		}
		else if (flags == XATTR_REPLACE && !xattr_exists) {
			rc = -ENOENT;
		}
		else {
			rc = 0;
		}
		if (rc != 0) {
			goto out;
		}
	}

	RC_WRAP_LABEL(rc, out, md_xattr_set, fs_ctx, (obj_id_t *)&oid, name,
		      value, size);

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s val=%p size=%lu",
		   fs_ctx, *ino, oid.f_hi, oid.f_lo, rc, name, value, size);
	return rc;
}

ssize_t efs_getxattr(efs_fs_ctx_t fs_ctx, efs_cred_t *cred,
		     const efs_ino_t *ino, const char *name, char *value,
		     size_t *size)
{
	int rc;
	dstore_oid_t oid;
	void *read_val = NULL;
	size_t size_val;

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, ino,
		      EFS_ACCESS_READ);

	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, fs_ctx, ino, &oid);

	RC_WRAP_LABEL(rc, out, md_xattr_get, fs_ctx, (obj_id_t *)&oid, name,
		      &read_val, &size_val);

	/* The caller/client wants to estimate the size of "value" */
	if (*size == 0) {
		rc = size_val;
		goto out;
	}

	/* The passed buffer size is insufficient to hold the xattr value */
	if (*size < size_val) {
		rc = -ERANGE;
		goto out;
	}

	memcpy(value, read_val, size_val);
	rc = size_val;

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s"
		   " val=%p size=%lu", fs_ctx, *ino, oid.f_hi, oid.f_lo, rc,
		   name, read_val, size_val);
	if (read_val != NULL) {
		md_xattr_free(read_val);
	}
	return rc;

}

int efs_removexattr(efs_ctx_t fs_ctx, const efs_cred_t *cred,
		    const efs_ino_t *ino, const char *name)
{
	int rc;
	dstore_oid_t oid;

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, ino,
		      EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, fs_ctx, ino, &oid);

	RC_WRAP_LABEL(rc, out, md_xattr_delete, fs_ctx, (obj_id_t *)&oid,
		      name);

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s",
		   fs_ctx, *ino, oid.f_hi, oid.f_lo, rc, name);
	return rc;
}

int efs_remove_all_xattr(efs_ctx_t fs_ctx, efs_cred_t *cred, efs_ino_t *ino)
{
	return 0;
}

int efs_listxattr(efs_ctx_t fs_ctx, const efs_cred_t *cred,
		  const efs_ino_t *ino, void *buf, size_t *count,
		  size_t *size)
{
	int rc;
	dstore_oid_t oid;

	dassert(cred != NULL);
	dassert(ino != NULL);
	dassert(buf != NULL);
	dassert(count != NULL);
	dassert(size != NULL);

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, ino,
		      EFS_ACCESS_READ);

	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, fs_ctx, ino, &oid);

	RC_WRAP_LABEL(rc, out, md_xattr_list, fs_ctx, (obj_id_t *)&oid, buf,
		      count, size);

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d",
		   fs_ctx, *ino, oid.f_hi, oid.f_lo, rc);
	return rc;
}

