/*
 * Filename:         cortxfs_fops.c
 * Description:      CORTXFS file system file operations
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

#include <string.h> /* memset */
#include <kvstore.h> /* kvstore */
#include <dstore.h> /* dstore */
#include <cortxfs.h> /* cfs_access */
#include "cortxfs_internal.h" /* cfs_set_ino_oid */
#include <common/log.h> /* log_* */
#include <common/helpers.h> /* RC_* */
#include <sys/param.h> /* DEV_SIZE */
#include "kvtree.h"

int cfs_creat(struct cfs_fs *cfs_fs, cfs_cred_t *cred, cfs_ino_t *parent,
	      char *name, mode_t mode, cfs_ino_t *newfile)
{
	dstore_oid_t  oid;
	struct dstore *dstore = dstore_get();

	dassert(dstore);

	log_trace("ENTER: parent=%p name=%s file=%p mode=0x%X",
		  parent, name, newfile, mode);

	RC_WRAP(cfs_access, cfs_fs, cred, parent, CFS_ACCESS_WRITE);
	/* Create tree entries, get new inode */
	RC_WRAP(cfs_create_entry, cfs_fs, cred, parent, name, NULL,
		mode, newfile, CFS_FT_FILE);
	/* Get new unique extstore kfid */
	RC_WRAP(dstore_get_new_objid, dstore, &oid);
	/* Set the ino-kfid key-val in kvs */
	RC_WRAP(cfs_set_ino_oid, cfs_fs, newfile, &oid);
	/* Create the backend object with passed kfid */
	RC_WRAP(dstore_obj_create, dstore, cfs_fs, &oid);
	log_trace("EXIT");
	return 0;
}

int cfs_creat_ex(struct cfs_fs *cfs_fs, cfs_cred_t *cred, cfs_ino_t *parent,
		 char *name, mode_t mode, struct stat *stat_in,
		 int stat_in_flags, cfs_ino_t *newfile,
		 struct stat *stat_out)
{
	int rc;
	cfs_ino_t object = 0;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(kvstor);

	index = cfs_fs->kvtree->index;

	/* NOTE: The following operations must be done within a single
	 * transaction.
	 */

	RC_WRAP(kvs_begin_transaction, kvstor, &index);

	RC_WRAP_LABEL(rc, out, cfs_creat, cfs_fs, cred, parent, name,
		      mode, &object);
	RC_WRAP_LABEL(rc, out, cfs_setattr, cfs_fs, cred, &object, stat_in,
		      stat_in_flags);
	RC_WRAP_LABEL(rc, out, cfs_getattr, cfs_fs, cred, &object, stat_out);

	RC_WRAP(kvs_end_transaction, kvstor, &index);

	*newfile = object;
	object = 0;

out:
	if (object != 0) {
		/* We don't have transactions, so that let's just remove the
		 * object.
		 */
		(void) cfs_unlink(cfs_fs, cred, parent, &object, name);
		(void) kvs_discard_transaction(kvstor, &index);
	}
	return rc;
}

ssize_t cfs_write(struct cfs_fs *cfs_fs, cfs_cred_t *cred, cfs_file_open_t *fd,
		  void *buf, size_t count, off_t offset)
{
	int rc;
	ssize_t write_amount;
	bool stable;
	struct stat stat;
	struct stat wstat;
	dstore_oid_t oid;
	struct dstore *dstore = dstore_get();
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(dstore);

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd,
		  count, (long)offset);

	memset(&wstat, 0, sizeof(wstat));

	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, &fd->ino, &oid);

	RC_WRAP(cfs_access, cfs_fs, cred, &fd->ino, CFS_ACCESS_WRITE);

	void *ctx = cfs_fs->kvtree->index.index_priv;
	write_amount = dstore_obj_write(dstore, ctx, &oid, offset,
					count, buf, &stable,
					&wstat);
	if (write_amount < 0) {
		rc = write_amount;
		goto out;
	}

	RC_WRAP(cfs_getattr, cfs_fs, cred, &fd->ino, &stat);
	if (wstat.st_size > stat.st_size) {
		stat.st_size = wstat.st_size;
		stat.st_blocks = wstat.st_blocks;
	}
	stat.st_mtim = wstat.st_mtim;
	stat.st_ctim = wstat.st_ctim;

	RC_WRAP_LABEL(rc, out, cfs_kvnode_init, &node, cfs_fs->kvtree, &fd->ino,
		      &stat);
	RC_WRAP_LABEL(rc, out, cfs_set_stat, &node);
	rc = write_amount;
out:
	kvnode_fini(&node);
	log_trace("cfs_write: rc=%d", rc);
	return rc;
}

int cfs_truncate(struct cfs_fs *cfs_fs, cfs_cred_t *cred, cfs_ino_t *ino,
		 struct stat *new_stat, int new_stat_flags)
{
	int rc;
	dstore_oid_t oid;
	struct dstore *dstore = dstore_get();
	struct stat stat;
	size_t old_size;
	size_t new_size;

	dassert(ino && new_stat && dstore);
	dassert((new_stat_flags & STAT_SIZE_SET) != 0);

	/* TODO:PERF: The caller can pass the current size */
	RC_WRAP_LABEL(rc, out, cfs_getattr, cfs_fs, cred, ino, &stat);

	old_size = stat.st_size;
	new_size = new_stat->st_size;
	new_stat->st_blocks = (new_size + DEV_BSIZE - 1) / DEV_BSIZE;

	/* If the caller wants to set mtime explicitly then
	 * mtime and ctime will be different. Othewise,
	 * we should keep them synchronous with each other.
	 */
	if ((new_stat_flags & STAT_MTIME_SET) == 0) {
		RC_WRAP_LABEL(rc, out, cfs_amend_stat, new_stat,
			      STAT_MTIME_SET | STAT_CTIME_SET);
		new_stat_flags |= (STAT_MTIME_SET | STAT_CTIME_SET);
	}

	RC_WRAP_LABEL(rc, out, cfs_setattr, cfs_fs, cred, ino, new_stat,
		      new_stat_flags);


	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, ino, &oid);
	RC_WRAP_LABEL(rc, out, dstore_obj_resize, dstore, cfs_fs,
		      &oid, old_size, new_size);

out:
	return rc;
}

ssize_t cfs_read(struct cfs_fs *cfs_fs, cfs_cred_t *cred, cfs_file_open_t *fd,
		 void *buf, size_t count, off_t offset)
{
	int rc;
	ssize_t read_amount;
	bool eof;
	struct stat stat;
	dstore_oid_t oid;
	struct dstore *dstore = dstore_get();
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(dstore);

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd,
		  count, (long)offset);

	RC_WRAP_LABEL(rc, out, cfs_ino_to_oid, cfs_fs, &fd->ino, &oid);
	RC_WRAP(cfs_getattr, cfs_fs, cred, &fd->ino, &stat);
	RC_WRAP(cfs_access, cfs_fs, cred, &fd->ino, CFS_ACCESS_READ);

	void *ctx = cfs_fs->kvtree->index.index_priv;
	read_amount = dstore_obj_read(dstore, ctx, &oid, offset,
				      count, buf, &eof, &stat);
	if (read_amount < 0) {
		rc = read_amount;
		goto out;
	}
	RC_WRAP_LABEL(rc, out, cfs_kvnode_init, &node, cfs_fs->kvtree, &fd->ino,
		      &stat);
	RC_WRAP_LABEL(rc, out, cfs_set_stat, &node);
	rc = read_amount;
out:
	kvnode_fini(&node);
	log_trace("cfs_read rc=%d", rc);
	return rc;
}

