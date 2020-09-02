/*
 * Filename:         cortxfs_xattr.c
 * Description:      CORTXFS file system XATTR interfaces
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

#include <common/log.h> /* log_*() */
#include <common/helpers.h> /* RC_LABEL*() */
#include <cortxfs.h> /* cfs_* */
#include <dstore.h> /* dstore_oid_t */
#include <md_xattr.h> /* md_xattr_exists */
#include "cortxfs_internal.h" /*cfs_ino_to_oid */
#include <errno.h> /* ERANGE */
#include <string.h> /* memcpy */
#include <sys/xattr.h> /* XATTR_CREATE */
#include "kvtree.h"

int cfs_setxattr(struct cfs_fs *cfs_fs, const cfs_cred_t *cred,
		 const cfs_ino_t *ino, const char *name, char *value,
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

	RC_WRAP_LABEL(rc, out, cfs_access, cfs_fs, cred, ino,
		      CFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, ino, &oid);

	if ((flags == XATTR_CREATE) || (flags == XATTR_REPLACE)) {
		RC_WRAP_LABEL(rc, out, md_xattr_exists, &(cfs_fs->kvtree->index)
		,
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

	RC_WRAP_LABEL(rc, out, md_xattr_set, &(cfs_fs->kvtree->index), (obj_id_t *)&oid, name,
		      value, size);

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s val=%p size=%lu",
		   cfs_fs, *ino, oid.f_hi, oid.f_lo, rc, name, value, size);
	return rc;
}

size_t cfs_getxattr(struct cfs_fs *cfs_fs, cfs_cred_t *cred,
		     const cfs_ino_t *ino, const char *name, char *value,
		     size_t *size)
{
	int rc;
	dstore_oid_t oid;
	void *read_val = NULL;
	size_t size_val;

	dassert(size != NULL);
	dassert(*size != 0);

	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, ino, &oid);

	RC_WRAP_LABEL(rc, out, md_xattr_get, &(cfs_fs->kvtree->index), (obj_id_t *)&oid, name,
		      &read_val, &size_val);

	/* The passed buffer size is insufficient to hold the xattr value */
	if (*size < size_val) {
		rc = -ERANGE;
		goto out;
	}

	memcpy(value, read_val, size_val);
	*size = size_val;

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s"
		   " val=%p size=%lu", cfs_fs, *ino, oid.f_hi, oid.f_lo, rc,
		   name, read_val, size_val);
	if (read_val != NULL) {
		md_xattr_free(read_val);
	}
	return rc;

}

int cfs_removexattr(struct cfs_fs *cfs_fs, const cfs_cred_t *cred,
		    const cfs_ino_t *ino, const char *name)
{
	int rc;
	dstore_oid_t oid;

	RC_WRAP_LABEL(rc, out, cfs_access, cfs_fs, cred, ino,
		      CFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, ino, &oid);

	RC_WRAP_LABEL(rc, out, md_xattr_delete, &(cfs_fs->kvtree->index), (obj_id_t *)&oid,
		      name);

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s",
		   cfs_fs, *ino, oid.f_hi, oid.f_lo, rc, name);
	return rc;
}

int cfs_remove_all_xattr(struct cfs_fs *cfs_fs, cfs_cred_t *cred, cfs_ino_t *ino)
{
	return 0;
}

int cfs_listxattr(struct cfs_fs *cfs_fs, const cfs_cred_t *cred,
		  const cfs_ino_t *ino, void *buf, size_t *count,
		  size_t *size)
{
	int rc;
	dstore_oid_t oid;

	dassert(cred != NULL);
	dassert(ino != NULL);
	dassert(buf != NULL);
	dassert(count != NULL);
	dassert(size != NULL);

	RC_WRAP_LABEL(rc, out, cfs_access, cfs_fs, cred, ino,
		      CFS_ACCESS_READ);

	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, ino, &oid);

	RC_WRAP_LABEL(rc, out, md_xattr_list, &(cfs_fs->kvtree->index), (obj_id_t *)&oid, buf,
		      count, size);

out:
	log_trace("ctx=%p *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d",
		   cfs_fs, *ino, oid.f_hi, oid.f_lo, rc);
	return rc;
}

