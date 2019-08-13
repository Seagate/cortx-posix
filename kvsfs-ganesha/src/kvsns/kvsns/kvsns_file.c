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

/* kvsns_file.c
 * KVSNS: functions related to file management
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syscall.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include <kvsns/extstore.h>
#include "kvsns_internal.h"
#include "kvsns/log.h"


int kvsns_creat(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		char *name, mode_t mode, kvsns_ino_t *newfile)
{
	kvsns_fid_t  kfid;


	log_trace("ENTER: parent=%p name=%s file=%p mode=0x%X",
		  parent, name, newfile, mode);

	RC_WRAP(kvsns2_access, ctx, cred, parent, KVSNS_ACCESS_WRITE);
	/* Create tree entries, get new inode */
	RC_WRAP(kvsns2_create_entry, ctx, cred, parent, name, NULL,
		mode, newfile, KVSNS_FILE);
	/* Get new unique extstore kfid */
	RC_WRAP(extstore_get_new_kfid, *newfile, &kfid);
	/* Set the ino-kfid key-val in kvs */
	RC_WRAP(kvsns_set_ino_kfid, ctx, newfile, &kfid);
	/* Create the backend object with passed kfid */
	RC_WRAP(extstore2_create, ctx, &kfid);
	log_trace("EXIT");
	return 0;
}

int kvsns_creat_ex(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		   char *name, mode_t mode, struct stat *stat_in,
		   int stat_in_flags, kvsns_ino_t *newfile,
		   struct stat *stat_out)
{
	int rc;
	kvsns_ino_t object = 0;

	/* NOTE: The following operations must be done within a single
	 * transaction.
	 */

	RC_WRAP(kvsal_begin_transaction);

	RC_WRAP_LABEL(rc, out, kvsns_creat, ctx, cred, parent, name,
		      mode, &object);
	RC_WRAP_LABEL(rc, out, kvsns2_setattr, ctx, cred, &object, stat_in,
		      stat_in_flags);
	RC_WRAP_LABEL(rc, out, kvsns2_getattr, ctx, cred, &object, stat_out);

	RC_WRAP(kvsal_end_transaction);

	*newfile = object;
	object = 0;

out:
	if (object != 0) {
		/* We don't have transactions, so that let's just remove the
		 * object.
		 */
		(void) kvsns2_unlink(ctx, cred, parent, name);
		// kvsal_discard_transaction();
	}
	return rc;
}

int kvsns_open(kvsns_cred_t *cred, kvsns_ino_t *ino,
	       int flags, mode_t mode, kvsns_file_open_t *fd)
{
	/* deprecated, see kvsns2_oepn for details */
	return 0;
}

int kvsns2_open(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		int flags, mode_t mode, kvsns_file_open_t *fd)
{
	char k[KLEN];
	int rc;
	int pid = getpid();
	int tid = syscall(SYS_gettid);
	int klen;

	log_trace("ENTER: ino=%llu fd=%p", *ino, fd);
	if (!cred || !ino || !fd) {
		rc = -EINVAL;
		goto out;
	}

	/* Manage the list of open owners */
	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.openowner.%d.%d", *ino, pid, tid);
	klen = rc;
	rc = kvsal2_exists(ctx, k, (size_t) klen);
	if (rc && rc != -ENOENT)
		goto out;
	else if (rc == -ENOENT)
		/* -ENOENT is desired value in case of open. Therefore, clean the rc */
		rc = 0;
	RC_WRAP(kvsal2_set_char, ctx, k, klen, "", 1);

	/** @todo Do not forget store stuffs */
	fd->ino = *ino;
	fd->owner.pid = pid;
	fd->owner.tid = tid;
	fd->flags = flags;

out:
	/* In particular create a key per opened fd */
	log_trace("Exit rc=%d", rc);
	return rc;
}

int kvsns_is_open(kvsns_fs_ctx_t *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		  bool *is_open)
{
	*is_open = false;
	return 0;
}

int kvsns_openat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 int flags, mode_t mode, kvsns_file_open_t *fd)
{
	kvsns_ino_t ino = 0LL;

	if (!cred || !parent || !name || !fd)
		return -EINVAL;

	RC_WRAP(kvsns_lookup, cred, parent, name, &ino);

	return kvsns_open(cred, &ino, flags, mode, fd);
}

int kvsns2_openat(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  int flags, mode_t mode, kvsns_file_open_t *fd)
{
	kvsns_ino_t ino = 0LL;

	if (!cred || !parent || !name || !fd)
		return -EINVAL;

	/* @todo: add context as a parameter in kvsns_lookup */
	// RC_WRAP(kvsns_lookup, cred, parent, name, &ino);

	return kvsns2_open(ctx, cred, &ino, flags, mode, fd);
}

int kvsns2_close(void *ctx, kvsns_file_open_t *fd)
{
	kvsns_fid_t  kfid;
	char k[KLEN];
	int klen;
	int rc;
	bool opened_and_deleted;
	bool delete_object = false;

	log_trace("ENTER: ino=%llu fd=%p", fd->ino, fd);
	if (!fd) {
		rc = -EINVAL;
		goto out;
	}

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.openowner.%d.%d", fd->ino, fd->owner.pid, fd->owner.tid);
	klen = rc;
	rc = kvsal2_del(ctx, k, klen);
	if (rc != 0) {
		if (rc == -ENOENT)
			rc = -EBADF; /* File was not opened */
		goto out;
	}

	/* Was the file deleted as it was opened ? */
	/* The last close should perform actual data deletion */
	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.opened_and_deleted", fd->ino);
	klen = rc;
	rc = kvsal2_exists(ctx, k, klen);
	if ((rc != 0) && (rc != -ENOENT))
		goto out;
	opened_and_deleted = (rc == -ENOENT) ? false : true;

	RC_WRAP(kvsal_begin_transaction);

	/* Was the file deleted as it was opened ? */
	if (opened_and_deleted) {
		/* Check if this is the last open file */
		RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.openowner.*", fd->ino);
		klen = rc;
		rc = kvsal2_get_list_size(ctx, k, klen);
		if (rc < 0) {
			goto out;
		} else if (rc > 0) {
			/* There are other entries, do not delete the object now */
			rc = 0;
			goto out;
		}

		delete_object = true;
		RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.opened_and_deleted", fd->ino);
		klen = rc;
		RC_WRAP_LABEL(rc, out, kvsal2_del, ctx, k, klen);
	}
	RC_WRAP(kvsal_end_transaction);

	if (delete_object) {
		RC_WRAP(kvsns_ino_to_kfid, ctx, &fd->ino, &kfid);
		RC_WRAP(extstore2_del, ctx, &kfid);
		RC_WRAP(kvsns_del_kfid, ctx, &fd->ino);
	}

	return 0;

out:
	kvsal_discard_transaction();
	log_trace("EXIT rc=%d", rc);
	return rc;
}


int kvsns_close(kvsns_file_open_t *fd)
{
	/* deprecated. See kvsns2_close for detais */
	return 0;
}

ssize_t kvsns_write(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		    void *buf, size_t count, off_t offset)
{
	ssize_t write_amount;
	bool stable;
	char k[KLEN];
	struct stat stat;
	struct stat wstat;

	memset(&wstat, 0, sizeof(wstat));

	/** @todo use flags to check correct access */
	write_amount = extstore_write(&fd->ino,
				      offset,
				      count,
				      buf,
				      &stable,
				      &wstat);
	if (write_amount < 0)
		return write_amount;

	RC_WRAP(kvsns_getattr, cred, &fd->ino, &stat);
	if (wstat.st_size > stat.st_size) {
		stat.st_size = wstat.st_size;
		stat.st_blocks = wstat.st_blocks;
	}
	stat.st_mtim = wstat.st_mtim;
	stat.st_ctim = wstat.st_ctim;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", fd->ino);
	RC_WRAP(kvsal_set_stat, k, &stat);

	return write_amount;
}

ssize_t kvsns2_write(void *ctx, kvsns_cred_t *cred, kvsns_file_open_t *fd,
		     void *buf, size_t count, off_t offset)
{
	int rc;
	ssize_t write_amount;
	bool stable;
	struct stat stat;
	struct stat wstat;
	kvsns_fid_t kfid;

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd, count, (long)offset);

	memset(&wstat, 0, sizeof(wstat));

	RC_WRAP_LABEL(rc, out, kvsns_ino_to_kfid, ctx, &fd->ino, &kfid);

	RC_WRAP(kvsns2_access, ctx, cred, &fd->ino, KVSNS_ACCESS_WRITE);
	write_amount = extstore2_write(ctx, &kfid, offset, count,
				       buf, &stable, &wstat);
	if (write_amount < 0) {
		rc = write_amount;
		goto out;
	}

	RC_WRAP(kvsns2_getattr, ctx, cred, &fd->ino, &stat);
	if (wstat.st_size > stat.st_size) {
		stat.st_size = wstat.st_size;
		stat.st_blocks = wstat.st_blocks;
	}
	stat.st_mtim = wstat.st_mtim;
	stat.st_ctim = wstat.st_ctim;

	RC_WRAP(kvsns2_set_stat, ctx, &fd->ino, &stat);
	rc = write_amount;
out:
	log_trace("EXIT rc=%d", rc);
	return rc;
}

ssize_t kvsns_read(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		   void *buf, size_t count, off_t offset)
{
	ssize_t read_amount;
	bool eof;
	struct stat stat;
	char k[KLEN];

	RC_WRAP(kvsns_getattr, cred, &fd->ino, &stat);

	/** @todo use flags to check correct access */
	read_amount = extstore_read(&fd->ino,
				    offset,
				    count,
				    buf,
				    &eof,
				    &stat);
	if (read_amount < 0)
		return read_amount;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", fd->ino);
	RC_WRAP(kvsal_set_stat, k, &stat);

	return read_amount;
}

ssize_t kvsns2_read(void *ctx, kvsns_cred_t *cred, kvsns_file_open_t *fd,
		    void *buf, size_t count, off_t offset)
{
	int rc;
	ssize_t read_amount;
	bool eof;
	struct stat stat;
	kvsns_fid_t kfid;

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd, count, (long)offset);

	RC_WRAP_LABEL(rc, out, kvsns_ino_to_kfid, ctx, &fd->ino, &kfid);
	RC_WRAP(kvsns2_getattr, ctx, cred, &fd->ino, &stat);
	RC_WRAP(kvsns2_access, ctx, cred, &fd->ino, KVSNS_ACCESS_READ);
	read_amount = extstore2_read(ctx, &kfid, offset, count,
				     buf, &eof, &stat);
	if (read_amount < 0) {
		rc = read_amount;
		goto out;
	}
	RC_WRAP(kvsns2_set_stat, ctx, &fd->ino, &stat);
	rc = read_amount;
out:
	log_trace("EXIT rc=%d", rc);
	return rc;
}

int kvsns_attach(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 char *objid, int objid_len, struct stat *stat, int statflags,
		  kvsns_ino_t *newfile)
{
	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);
	RC_WRAP(kvsns_create_entry, cred, parent, name, NULL,
				    stat->st_mode, newfile, KVSNS_FILE);
	RC_WRAP(kvsns_setattr, cred, newfile, stat, statflags);
	RC_WRAP(kvsns_getattr, cred, newfile, stat);
	RC_WRAP(extstore_attach, newfile, objid, objid_len);

	return 0;
}
