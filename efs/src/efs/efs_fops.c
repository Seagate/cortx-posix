/*
 * Filename:         efs_fops.c
 * Description:      EOS file system file operations

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system file operations
*/

#include <string.h> /* memset */
#include <kvstore.h> /* kvstore */
#include <dstore.h> /* dstore */
#include <efs.h> /* efs_access */
#include "efs_internal.h" /* efs_set_ino_oid */
#include <common/log.h> /* log_* */
#include <common/helpers.h> /* RC_* */
#include <sys/param.h> /* DEV_SIZE */

int efs_creat(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *parent,
	      char *name, mode_t mode, efs_ino_t *newfile)
{
	dstore_oid_t  oid;
	struct dstore *dstore = dstore_get();

	dassert(dstore);

	log_trace("ENTER: parent=%p name=%s file=%p mode=0x%X",
		  parent, name, newfile, mode);

	RC_WRAP(efs_access, ctx, cred, parent, EFS_ACCESS_WRITE);
	/* Create tree entries, get new inode */
	RC_WRAP(efs_create_entry, ctx, cred, parent, name, NULL,
		mode, newfile, EFS_FT_FILE);
	/* Get new unique extstore kfid */
	RC_WRAP(dstore_get_new_objid, dstore, &oid);
	/* Set the ino-kfid key-val in kvs */
	RC_WRAP(efs_set_ino_oid, ctx, newfile, &oid);
	/* Create the backend object with passed kfid */
	RC_WRAP(dstore_obj_create, dstore, ctx, &oid);
	log_trace("EXIT");
	return 0;
}

int efs_creat_ex(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *parent,
		 char *name, mode_t mode, struct stat *stat_in,
		 int stat_in_flags, efs_ino_t *newfile,
		 struct stat *stat_out)
{
	int rc;
	efs_ino_t object = 0;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(kvstor);

	index.index_priv = ctx;

	/* NOTE: The following operations must be done within a single
	 * transaction.
	 */

	RC_WRAP(kvs_begin_transaction, kvstor, &index);

	RC_WRAP_LABEL(rc, out, efs_creat, ctx, cred, parent, name,
		      mode, &object);
	RC_WRAP_LABEL(rc, out, efs_setattr, ctx, cred, &object, stat_in,
		      stat_in_flags);
	RC_WRAP_LABEL(rc, out, efs_getattr, ctx, cred, &object, stat_out);

	RC_WRAP(kvs_end_transaction, kvstor, &index);

	*newfile = object;
	object = 0;

out:
	if (object != 0) {
		/* We don't have transactions, so that let's just remove the
		 * object.
		 */
		(void) efs_unlink(ctx, cred, parent, &object, name);
		(void) kvs_discard_transaction(kvstor, &index);
	}
	return rc;
}

ssize_t efs_write(efs_ctx_t ctx, efs_cred_t *cred, efs_file_open_t *fd,
		  void *buf, size_t count, off_t offset)
{
	int rc;
	ssize_t write_amount;
	bool stable;
	struct stat stat;
	struct stat wstat;
	dstore_oid_t oid;
	struct dstore *dstore = dstore_get();

	dassert(dstore);

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd, count, (long)offset);

	memset(&wstat, 0, sizeof(wstat));

	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, ctx, &fd->ino, &oid);

	RC_WRAP(efs_access, ctx, cred, &fd->ino, EFS_ACCESS_WRITE);
	write_amount = dstore_obj_write(dstore, ctx, &oid, offset,
					count, buf, &stable,
					&wstat);
	if (write_amount < 0) {
		rc = write_amount;
		goto out;
	}

	RC_WRAP(efs_getattr, ctx, cred, &fd->ino, &stat);
	if (wstat.st_size > stat.st_size) {
		stat.st_size = wstat.st_size;
		stat.st_blocks = wstat.st_blocks;
	}
	stat.st_mtim = wstat.st_mtim;
	stat.st_ctim = wstat.st_ctim;

	RC_WRAP(efs_set_stat, ctx, &fd->ino, &stat);
	rc = write_amount;
out:
	log_trace("EXIT rc=%d", rc);
	return rc;
}

int efs_truncate(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *ino,
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
	RC_WRAP_LABEL(rc, out, efs_getattr, ctx, cred, ino, &stat);

	old_size = stat.st_size;
	new_size = new_stat->st_size;
	new_stat->st_blocks = (new_size + DEV_BSIZE - 1) / DEV_BSIZE;

	/* If the caller wants to set mtime explicitly then
	 * mtime and ctime will be different. Othewise,
	 * we should keep them synchronous with each other.
	 */
	if ((new_stat_flags & STAT_MTIME_SET) == 0) {
		RC_WRAP_LABEL(rc, out, efs_amend_stat, new_stat,
			      STAT_MTIME_SET | STAT_CTIME_SET);
		new_stat_flags |= (STAT_MTIME_SET | STAT_CTIME_SET);
	}

	RC_WRAP_LABEL(rc, out, efs_setattr, ctx, cred, ino, new_stat,
		      new_stat_flags);


	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, ctx, ino, &oid);
	RC_WRAP_LABEL(rc, out, dstore_obj_resize, dstore, ctx,
		      &oid, old_size, new_size);

out:
	return rc;
}

ssize_t efs_read(efs_ctx_t ctx, efs_cred_t *cred, efs_file_open_t *fd,
		 void *buf, size_t count, off_t offset)
{
	int rc;
	ssize_t read_amount;
	bool eof;
	struct stat stat;
	dstore_oid_t oid;
	struct dstore *dstore = dstore_get();

	dassert(dstore);

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd, count, (long)offset);

	RC_WRAP_LABEL(rc, out, efs_ino_to_oid, ctx, &fd->ino, &oid);
	RC_WRAP(efs_getattr, ctx, cred, &fd->ino, &stat);
	RC_WRAP(efs_access, ctx, cred, &fd->ino, EFS_ACCESS_READ);
	read_amount = dstore_obj_read(dstore, ctx, &oid, offset,
				      count, buf, &eof, &stat);
	if (read_amount < 0) {
		rc = read_amount;
		goto out;
	}
	RC_WRAP(efs_set_stat, ctx, &fd->ino, &stat);
	rc = read_amount;
out:
	log_trace("EXIT rc=%d", rc);
	return rc;
}

