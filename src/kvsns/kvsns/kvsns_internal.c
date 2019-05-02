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

/* kvsns_internal.c
 * KVSNS: set of internal functions
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <linux/limits.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include "kvsns_internal.h"
#include "kvsns/log.h"

int kvsns_next_inode(kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -EINVAL;

	rc = kvsal_incr_counter("ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns2_next_inode(void *ctx, kvsns_ino_t *ino)
{
	int rc;
	if (!ino)
		return -EINVAL;

	rc = kvsal2_incr_counter(ctx, "ino_counter", ino);
	if (rc != 0)
		return rc;

	return 0;
}

int kvsns_get_fd(void *ctx, kvsns_ino_t *ino, int *fd)
{
	int rc;
	int cur_fd, max_fd = KVSNS_MAX_FD;
	char k[KLEN];
	char v[VLEN];
	size_t vlen = VLEN;
	size_t klen;

	if (!ino)
		return -EINVAL;

	/* Get the current fd counter for corresponding inode */
	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.fd_counter", *ino);
	klen = rc;

	RC_WRAP_LABEL(rc, out, kvsal2_get_char, ctx, k, klen, v, vlen);
	sscanf(v, "%d", &cur_fd);

	if (cur_fd <= max_fd) {
		int fd_counter = cur_fd;
		while (cur_fd < max_fd) {
			/* Check until we get the unused fd number */
			RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.openowner.%d", *ino, cur_fd);
			klen = rc;
			rc = kvsal2_exists(ctx, k, (size_t) klen);
			if (rc && rc != -ENOENT)
				goto out;
			else if (rc == -ENOENT) {
				/* cur_fd is not in use, therefore return current fd */
				*fd = cur_fd;

				RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.fd_counter", *ino);
				klen = rc;
				RC_WRAP_LABEL(rc, out, prepare_key, v, VLEN, "%d", *fd);
				vlen = rc;
				RC_WRAP_LABEL(rc, out, kvsal2_set_char, ctx, k, klen, v, vlen);
				rc = 0;
				goto out;
			}
			cur_fd++;
		}
		/* We have traversed all the FD’s from fd_counter to KVSNS_MAX_FD.
		 * Now we need to traverse from 0 to fd_counter  */
		max_fd = fd_counter;
	}

	/* We have reached max_fd, start from 0 to get unused fd */
	cur_fd = 0;
	while (cur_fd < max_fd) {
		/* Check until we get the unused fd number */
		RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.openowner.%d", *ino, cur_fd);
		klen = rc;
		rc = kvsal2_exists(ctx, k, (size_t) klen);
		if (rc && rc != -ENOENT)
			goto out;
		else if (rc == -ENOENT) {
			/* cur_fd is not in use, therefore return current fd */
			*fd = cur_fd;

			RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.fd_counter", *ino);
			klen = rc;
			RC_WRAP_LABEL(rc, out, prepare_key, v, VLEN, "%d", *fd);
			vlen = rc;
			RC_WRAP_LABEL(rc, out, kvsal2_set_char, ctx, k, klen, v, vlen);
			rc = 0;
			goto out;
		}
		cur_fd++;
	}
	/* We have exhausted all the fds therefore return too many open files*/
	rc = -EMFILE;
out:
	log_trace("Exit rc=%d", rc);
	return rc;
}

int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str)
{
	char *token;
	char *rest = str;
	int maxsize = 0;
	int pos = 0;

	if (!inolist || !str || !size)
		return -EINVAL;

	maxsize = *size;

	while ((token = strtok_r(rest, "|", &rest))) {
		sscanf(token, "%llu", &inolist[pos++]);

		if (pos == maxsize)
			break;
	}

	*size = pos;

	return 0;
}

int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str)
{
	int i;
	char tmp[VLEN];

	if (!inolist || !str)
		return -EINVAL;

	strcpy(str, "");

	for (i = 0; i < size ; i++)
		if (inolist[i] != 0LL) {
			snprintf(tmp, VLEN, "%llu|", inolist[i]);
			strcat(str, tmp);
		}

	return 0;
}

int kvsns_update_stat(kvsns_ino_t *ino, int flags)
{
	char k[KLEN];
	struct stat stat;

	if (!ino)
		return -EINVAL;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", *ino);
	RC_WRAP(kvsal_get_stat, k, &stat);
	RC_WRAP(kvsns_amend_stat, &stat, flags);
	RC_WRAP(kvsal_set_stat, k, &stat);

	return 0;
}

int kvsns_amend_stat(struct stat *stat, int flags)
{
	struct timeval t;

	if (!stat)
		return -EINVAL;

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	if (flags & STAT_ATIME_SET) {
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_MTIME_SET) {
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_CTIME_SET) {
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_INCR_LINK)
		stat->st_nlink += 1;

	if (flags & STAT_DECR_LINK) {
		if (stat->st_nlink == 1)
			return -EINVAL;

		stat->st_nlink -= 1;
	}
	return 0;
}

/* @todo : Modify this for other operations like rename/delete. */
static int kvsns_create_check_name(const char *name, size_t len)
{
	const char *parent = "..";

	if (len >= NAME_MAX) {
		log_debug("Name too long %s", name);
		return  -E2BIG;
	}

	if (len == 1 && (name[0] == '.' || name[0] == '/')) {
		log_debug("File already exists: %s", name);
		return -EEXIST;
	}

	if (len == 2 && (strncmp(name, parent, 2) == 0)) {
		log_debug("File already exists: %s", name);
		return -EEXIST;
	}

	return 0;
}

int kvsns2_create_entry(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
			char *name, char *lnk, mode_t mode,
			kvsns_ino_t *new_entry, enum kvsns_type type)
{
	int	rc;
	char	k[KLEN];
	char	v[KLEN];
	struct	stat bufstat;
	struct	timeval t;
	size_t	klen;
	size_t vlen;
	size_t	namelen;
	struct stat parent_stat;

	if (!cred || !parent || !name || !new_entry)
		return -EINVAL;

	namelen = strlen(name);
	if (namelen == 0)
		return -EINVAL;

	/* check if name is not '.' or '..' or '/' */
	rc = kvsns_create_check_name(name, namelen);
	if (rc != 0)
		return rc;

	if ((type == KVSNS_SYMLINK) && (lnk == NULL))
		return -EINVAL;

	/* Return if file/dir/symlink already exists. */
	rc = kvsns2_lookup(ctx, cred, parent, name, new_entry);
	if (rc == 0)
		return -EEXIST;


	RC_WRAP(kvsns2_get_stat, ctx, parent, &parent_stat);

	RC_WRAP(kvsal_begin_transaction);

	/* Add an entry to parent dentries list */
	RC_WRAP_LABEL(rc, aborted, prepare_key, k, KLEN, "%llu.dentries.%s",
		      *parent, name);
	klen = rc;
	/* Get a new inode number for the new file and set it */
	RC_WRAP(kvsns2_next_inode, ctx, new_entry);

	/* @todo: Release the inode in case any of the calls below fail. */
	RC_WRAP_LABEL(rc, aborted, prepare_key, v, VLEN, "%llu", *new_entry);
	vlen = rc;
	RC_WRAP_LABEL(rc, aborted, kvsal2_set_char, ctx, k, klen, v, vlen);

	/* Set the parentdir of the new file */
	RC_WRAP_LABEL(rc, aborted, prepare_key, k, KLEN, "%llu.parentdir.%llu",
		      *new_entry, *parent);
	klen = rc;
	RC_WRAP_LABEL(rc, aborted, prepare_key, v, VLEN, "%llu", *parent);
	vlen = rc;
	RC_WRAP_LABEL(rc, aborted, kvsal2_set_char, ctx,  k, klen, v, vlen);

	RC_WRAP_LABEL(rc, aborted, prepare_key, k, KLEN, "%llu.fd_counter", *new_entry);
	klen = rc;
	RC_WRAP_LABEL(rc, aborted, kvsal2_set_char, ctx, k, klen, "0", 2 /* "0" */);

	/* Set the stats of the new file */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_uid = cred->uid;
	bufstat.st_gid = cred->gid;
	bufstat.st_ino = *new_entry;

	if (gettimeofday(&t, NULL) != 0) {
		rc = -1;
		goto aborted;
	}

	bufstat.st_atim.tv_sec = t.tv_sec;
	bufstat.st_atim.tv_nsec = 1000 * t.tv_usec;

	bufstat.st_mtim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_mtim.tv_nsec = bufstat.st_atim.tv_nsec;

	bufstat.st_ctim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_ctim.tv_nsec = bufstat.st_atim.tv_nsec;

	switch (type) {
	case KVSNS_DIR:
		bufstat.st_mode = S_IFDIR|mode;
		bufstat.st_nlink = 2;
		break;

	case KVSNS_FILE:
		bufstat.st_mode = S_IFREG|mode;
		bufstat.st_nlink = 1;
		break;

	case KVSNS_SYMLINK:
		bufstat.st_mode = S_IFLNK|mode;
		bufstat.st_nlink = 1;
		break;

	default:
		rc = -EINVAL;
		goto aborted;
	}
	RC_WRAP_LABEL(rc, aborted, prepare_key, k, KLEN, "%llu.stat",
		      *new_entry);
	klen = rc;

	RC_WRAP_LABEL(rc, aborted, kvsal2_set_stat, ctx, k, klen, &bufstat);

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns2_set_stat, ctx, parent, &parent_stat);
	RC_WRAP(kvsal_end_transaction);
	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

int kvsns_create_entry(kvsns_cred_t *cred, kvsns_ino_t *parent,
		       char *name, char *lnk, mode_t mode,
		       kvsns_ino_t *new_entry, enum kvsns_type type)
{
	int rc;
	char k[KLEN];
	char v[KLEN];
	struct stat bufstat;
	struct stat parent_stat;
	struct timeval t;

	if (!cred || !parent || !name || !new_entry)
		return -EINVAL;

	if ((type == KVSNS_SYMLINK) && (lnk == NULL))
		return -EINVAL;

	rc = kvsns_lookup(cred, parent, name, new_entry);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP(kvsns_next_inode, new_entry);
	RC_WRAP(kvsns_get_stat, parent, &parent_stat);

	RC_WRAP(kvsal_begin_transaction);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.dentries.%s",
		 *parent, name);
	snprintf(v, VLEN, "%llu", *new_entry);

	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.parentdir", *new_entry);
	snprintf(v, VLEN, "%llu|", *parent);

	RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_uid = cred->uid; 
	bufstat.st_gid = cred->gid;
	bufstat.st_ino = *new_entry;

	if (gettimeofday(&t, NULL) != 0)
		return -1;

	bufstat.st_atim.tv_sec = t.tv_sec;
	bufstat.st_atim.tv_nsec = 1000 * t.tv_usec;

	bufstat.st_mtim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_mtim.tv_nsec = bufstat.st_atim.tv_nsec;

	bufstat.st_ctim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_ctim.tv_nsec = bufstat.st_atim.tv_nsec;

	switch (type) {
	case KVSNS_DIR:
		bufstat.st_mode = S_IFDIR|mode;
		bufstat.st_nlink = 2;
		break;

	case KVSNS_FILE:
		bufstat.st_mode = S_IFREG|mode;
		bufstat.st_nlink = 1;
		break;

	case KVSNS_SYMLINK:
		bufstat.st_mode = S_IFLNK|mode;
		bufstat.st_nlink = 1;
		break;

	default:
		return -EINVAL;
	}
	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", *new_entry);
	RC_WRAP_LABEL(rc, aborted, kvsal_set_stat, k, &bufstat);

	if (type == KVSNS_SYMLINK) {
		memset(k, 0, KLEN);
		snprintf(k, KLEN, "%llu.link", *new_entry);
		RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, lnk);
	}

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, parent, &parent_stat);

	RC_WRAP(kvsal_end_transaction);
	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

/* Access routines */
static int kvsns_access_check(kvsns_cred_t *cred, struct stat *stat, int flags)
{
	int check = 0;

	if (!stat || !cred)
		return -EINVAL;

	/* Root's superpowers */
	if (cred->uid == KVSNS_ROOT_UID)
		return 0;

	if (cred->uid == stat->st_uid) {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_OWNER_READ;

		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_OWNER_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_OWNER_EXEC;
	} else if (cred->gid == stat->st_gid) {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_GROUP_READ;

		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_GROUP_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_GROUP_EXEC;
	} else {
		if (flags & KVSNS_ACCESS_READ)
			check |= STAT_OTHER_READ;

		if (flags & KVSNS_ACCESS_WRITE)
			check |= STAT_OTHER_WRITE;

		if (flags & KVSNS_ACCESS_EXEC)
			check |= STAT_OTHER_EXEC;
	}

	if ((check & stat->st_mode) != check)
		return -EPERM;
	else
		return 0;

	/* Should not be reached */
	return -EPERM;
}

int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags)
{
	struct stat stat;

	if (!cred || !ino)
		return -EINVAL;

	RC_WRAP(kvsns_getattr, cred, ino, &stat);

	return kvsns_access_check(cred, &stat, flags);
}

int kvsns2_access(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino, int flags)
{
	struct stat stat;

	if (!cred || !ino)
		return -EINVAL;

	RC_WRAP(kvsns2_getattr, ctx, cred, ino, &stat);

	return kvsns_access_check(cred, &stat, flags);
}

int kvsns_get_stat(kvsns_ino_t *ino, struct stat *bufstat)
{
	char k[KLEN];

	if (!ino || !bufstat)
		return -EINVAL;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvsal_get_stat(k, bufstat);
}

int kvsns2_get_stat(void *ctx, kvsns_ino_t *ino, struct stat *bufstat)
{
	int rc;
	char k[KLEN];
	size_t klen;

	if (!ino || !bufstat)
		return -EINVAL;

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.stat", *ino);
	klen = rc;
	RC_WRAP_LABEL(rc, out, kvsal2_get_stat, ctx, k, klen, bufstat);

out:
	return rc;

}


int kvsns_set_stat(kvsns_ino_t *ino, struct stat *bufstat)
{
	char k[KLEN];

	if (!ino || !bufstat)
		return -EINVAL;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvsal_set_stat(k, bufstat);
}

int kvsns2_set_stat(void *ctx, kvsns_ino_t *ino, struct stat *bufstat)
{
	char k[KLEN];
	int rc;
	size_t klen;

	if (!ino || !bufstat) {
		rc = -EINVAL;
		goto out;
	}

	RC_WRAP_LABEL(rc, out, prepare_key, k, KLEN, "%llu.stat", *ino);
	klen = rc;
	RC_WRAP_LABEL(rc, out, kvsal2_set_stat, ctx, k, klen, bufstat);

out:
	return rc;
}

int kvsns_lookup_path(kvsns_cred_t *cred, kvsns_ino_t *parent, char *path,
		       kvsns_ino_t *ino)
{
	char *saveptr;
	char *str;
	char *token;
	kvsns_ino_t iter_ino = 0LL;
	kvsns_ino_t *iter = NULL;
	int j = 0;
	int rc = 0;

	memcpy(&iter_ino, parent, sizeof(kvsns_ino_t));
	iter = &iter_ino;
	for (j = 1, str = path; ; j++, str = NULL) {
		memcpy(parent, ino, sizeof(kvsns_ino_t));
		token = strtok_r(str, "/", &saveptr);
		if (token == NULL)
			break;

		rc = kvsns_lookup(cred, iter, token, ino);
		if (rc != 0) {
			if (rc == -ENOENT)
				break;
			else
				return rc;
		}

		iter = ino;
	}

	if (token != NULL) /* If non-existing file should be created */
		strcpy(path, token);

	return rc;
}

