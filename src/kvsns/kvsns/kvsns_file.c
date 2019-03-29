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

static int kvsns_str2ownerlist(kvsns_open_owner_t *ownerlist, int *size,
			        char *str)
{
	char *token;
	char *rest = str;
	int maxsize;
	int pos;

	if (!ownerlist || !str || !size)
		return -EINVAL;

	maxsize = *size;
	pos = 0;

	while((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%u.%u",
		       &ownerlist[pos].pid,
		       &ownerlist[pos].tid);
		pos += 1;
		if (pos == maxsize)
			break;
	}

	*size = pos;

	return 0;
}

static int kvsns_ownerlist2str(kvsns_open_owner_t *ownerlist, int size,
			       char *str)
{
	int i;
	char tmp[VLEN];

	if (!ownerlist || !str)
		return -EINVAL;

	strcpy(str, "");

	for (i = 0; i < size ; i++)
		if (ownerlist[i].pid != 0LL) {
			snprintf(tmp, VLEN, "%u.%u|",
				 ownerlist[i].pid, ownerlist[i].tid);
			strcat(str, tmp);
		}

	return 0;
}


int kvsns_creat(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newfile)
{
	struct stat stat;

	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);
	RC_WRAP(kvsns_create_entry, cred, parent, name, NULL,
				  mode, newfile, KVSNS_FILE);
	RC_WRAP(kvsns_get_stat, newfile, &stat);
	RC_WRAP(extstore_create, *newfile);

	return 0;
}

int kvsns2_creat(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		mode_t mode, kvsns_ino_t *newfile)
{
	kvsns_fid_t  kfid;

	/*@todo Check write access to the directory */

	log_trace("ENTER: parent=%p name=%s file=%p mode=0x%X",
		  parent, name, newfile, mode);
	RC_WRAP(kvsns2_create_entry, ctx, cred, parent, name, NULL,
		mode, newfile, KVSNS_FILE);
	RC_WRAP(extstore_get_fid, *newfile, &kfid);
	RC_WRAP(extstore2_create, ctx, *newfile, &kfid);
	log_trace("EXIT");
	return 0;
}

int kvsns_open(kvsns_cred_t *cred, kvsns_ino_t *ino,
	       int flags, mode_t mode, kvsns_file_open_t *fd)
{
	kvsns_open_owner_t me;
	kvsns_open_owner_t owners[KVSAL_ARRAY_SIZE];
	int size = KVSAL_ARRAY_SIZE;
	char k[KLEN];
	char v[VLEN];
	int rc;

	if (!cred || !ino || !fd)
		return -EINVAL;

	/** @todo Put here the access control base on flags and mode values */
	me.pid = getpid();
	me.tid = syscall(SYS_gettid);

	/* Manage the list of open owners */
	memset(k, 0, KLEN);
	memset(v, 0, VLEN);
	snprintf(k, KLEN, "%llu.openowner", *ino);
	rc = kvsal_get_char(k, v);
	if (rc == 0) {
		RC_WRAP(kvsns_str2ownerlist, owners, &size, v);
		if (size == KVSAL_ARRAY_SIZE)
			return -EMLINK; /* Too many open files */
		owners[size].pid = me.pid;
		owners[size].tid = me.tid;
		size += 1;
		RC_WRAP(kvsns_ownerlist2str, owners, size, v);
	} else if (rc == -ENOENT) {
		/* Create the key => 1st fd created */
		snprintf(v, VLEN, "%u.%u|", me.pid, me.tid);
	} else
		return rc;

	RC_WRAP(kvsal_set_char, k, v);

	/** @todo Do not forget store stuffs */
	fd->ino = *ino;
	fd->owner.pid = me.pid;
	fd->owner.tid = me.tid;
	fd->flags = flags;

	/* In particular create a key per opened fd */

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
		RC_WRAP(extstore_get_fid, fd->ino, &kfid);
		RC_WRAP(extstore2_del, ctx, &fd->ino, &kfid);
	}

	return 0;

out:
	kvsal_discard_transaction();
	log_trace("EXIT rc=%d", rc);
	return rc;
}


int kvsns_close(kvsns_file_open_t *fd)
{
	kvsns_open_owner_t owners[KVSAL_ARRAY_SIZE];
	int size = KVSAL_ARRAY_SIZE;
	char k[KLEN];
	char v[VLEN];
	int i;
	int rc;
	bool found = false;
	bool opened_and_deleted;
	bool delete_object = false;

	if (!fd)
		return -EINVAL;

	memset(k, 0, KLEN);
	memset(v, 0, VLEN);
	snprintf(k, KLEN, "%llu.openowner", fd->ino);
	rc = kvsal_get_char(k, v);
	if (rc != 0) {
		if (rc == -ENOENT)
			return -EBADF; /* File not opened */
		else
			return rc;
	}

	/* Was the file deleted as it was opened ? */
	/* The last close should perform actual data deletion */
	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.opened_and_deleted", fd->ino);
	rc = kvsal_exists(k);
	if ((rc != 0) && (rc != -ENOENT))
		return rc;
	opened_and_deleted = (rc == -ENOENT) ? false : true;

	RC_WRAP(kvsns_str2ownerlist, owners, &size, v);

	RC_WRAP(kvsal_begin_transaction);

	if (size == 1) {
		if (fd->owner.pid == owners[0].pid &&
		    fd->owner.tid == owners[0].tid) {
			memset(k, 0, KLEN);
			snprintf(k, KLEN, "%llu.openowner", fd->ino);
			RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

			/* Was the file deleted as it was opened ? */
			if (opened_and_deleted) {
				delete_object = true;
				snprintf(k, KLEN, "%llu.opened_and_deleted",
					 fd->ino);
				RC_WRAP_LABEL(rc, aborted,
					      kvsal_del, k);
			}
			RC_WRAP(kvsal_end_transaction);

			if (delete_object)
				RC_WRAP(extstore_del, &fd->ino);

			return 0;
		} else {
			rc = -EBADF;
			goto aborted;
		}
	} else {
		found = false;
		for (i = 0; i < size ; i++)
			if (owners[i].pid == fd->owner.pid &&
			    owners[i].tid == fd->owner.tid) {
				owners[i].pid = 0; /* remove it from list */
				found = true;
				break;
			}
	}


	if (!found) {
		rc = -EBADF;
		goto aborted;
	}

	RC_WRAP_LABEL(rc, aborted, kvsns_ownerlist2str, owners, size, v);

	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	RC_WRAP(kvsal_end_transaction);

	/* To be done outside of the previous transaction */
	if (delete_object)
		RC_WRAP(extstore_del, &fd->ino);

	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
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
	char k[KLEN];
	size_t klen;
	struct stat stat;
	struct stat wstat;
	kvsns_fid_t kfid;

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd, count, (long)offset);

	memset(&wstat, 0, sizeof(wstat));

	RC_WRAP_LABEL(rc, out, extstore_ino_to_fid, ctx, fd->ino, &kfid);

	/** @todo use flags to check correct access */
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

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.stat", fd->ino);
	klen = rc;
	RC_WRAP(kvsal2_set_stat, ctx, k, klen, &stat);
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
	char k[KLEN];
	size_t klen;
	kvsns_fid_t kfid;

	log_trace("ENTER: ino=%llu fd=%p count=%lu offset=%ld", fd->ino, fd, count, (long)offset);

	RC_WRAP_LABEL(rc, out, extstore_ino_to_fid, ctx, fd->ino, &kfid);
	RC_WRAP(kvsns2_getattr, ctx, cred, &fd->ino, &stat);
	/** @todo use flags to check correct access */
	read_amount = extstore2_read(ctx, &kfid, offset, count,
				     buf, &eof, &stat);
	if (read_amount < 0) {
		rc = read_amount;
		goto out;
	}

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.stat", fd->ino);
	klen = rc;
	RC_WRAP(kvsal2_set_stat, ctx, k, klen, &stat);
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
