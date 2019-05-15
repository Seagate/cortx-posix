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

int _kvsns_ns_prepare_inode_key(kvsns_key_type_t type, kvsns_ino_t ino, kvsns_inode_key_t *key)
{
	KVSNS_DASSERT(key != NULL);
	KVSNS_DASSERT(ver <= KVSNS_VER_INVALID);

	memset(key, 0, sizeof(kvsns_inode_key_t));
	key->k_ver = KVSNS_VER_0;
	key->k_type = type;
	key->k_inode = ino;

	log_debug("inode=%llu type=%d", ino, type);
	return sizeof(kvsns_inode_key_t);
}

uint64_t _kvsns_get_dirent_key_len(const uint8_t namelen)
{
	uint64_t klen;

	klen = (sizeof (kvsns_dentry_key_t) - sizeof (kvsns_name_t)) + namelen;
	log_debug("klen=%lu", klen);

	return klen;
}

void _kvsns_name_copy(const char *name, uint8_t len, kvsns_name_t *k_name)
{

	KVSNS_DASSERT(k_name  && name && (namelen > 0));

	k_name->s_len = len;
	memcpy(k_name->s_str, name, k_name->s_len);
	log_debug("s_str=%.*s, s_len=%u", k_name->s_len, k_name->s_str,
	           k_name->s_len);
}

int _kvsns_prepare_dirent_key(const kvsns_ino_t dino, uint8_t namelen,
			      const char *name, kvsns_dentry_key_t *key)
{
	int rc;

	KVSNS_DASSERT(key && name && (namelen > 0));

	memset(key, 0, sizeof(kvsns_dentry_key_t));
	key->d_ver = KVSNS_VER_0;
	key->d_type = KVSNS_KEY_DIRENT;
	key->d_inode = dino;

	 _kvsns_name_copy(name, namelen, &key->d_name);
	rc = _kvsns_get_dirent_key_len(namelen);

	log_debug("inode=%llu name=%.*s rc=%d", key->d_inode, key->d_name.s_len,
		  key->d_name.s_str, rc);
	return rc;
}

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

	if (len > NAME_MAX) {
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

/* @todo : Replace void *ctx with kvsns_fs_ctx_t */
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
	size_t  vlen;
	size_t	namelen;
	struct  stat parent_stat;
	kvsns_dentry_key_t d_key;

	/* @todo use KVSNS_DASSERT here */
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

	RC_WRAP(kvsns2_ns_get_stat, ctx, parent, &parent_stat);

	RC_WRAP(kvsal_begin_transaction);

	/* @todo: Alloc mero bufvecs and use it for key to avoid extra mem copy
	RC_WRAP_LABEL(rc, aborted, kvsns_alloc_dirent_key, namelen, &d_key); */

	/* Create dentry key and add it to parent dentries list. */
	RC_WRAP_LABEL(rc, aborted, _kvsns_prepare_dirent_key, *parent, namelen,
		      name, &d_key);
	klen = rc;
	log_debug("d_key size=%lu", klen);
	/* Get a new inode number for the new file and set it as val for
	   dentry key */
	RC_WRAP(kvsns2_next_inode, ctx, new_entry);

	/* @todo: Release the inode in case any of the calls below fail.*/
	RC_WRAP_LABEL(rc, aborted, kvsal2_set_bin, ctx, &d_key, klen, new_entry,
		      sizeof new_entry);

	/* Set the parentdir of the new file */
	RC_WRAP_LABEL(rc, aborted, prepare_key, k, KLEN, "%llu.parentdir.%llu",
		      *new_entry, *parent);
	klen = rc;
	RC_WRAP_LABEL(rc, aborted, prepare_key, v, VLEN, "%llu", *parent);
	vlen = rc;
	RC_WRAP_LABEL(rc, aborted, kvsal2_set_char, ctx,  k, klen, v, vlen);

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
	RC_WRAP_LABEL(rc, aborted, kvsns2_ns_set_stat, ctx, new_entry, &bufstat);
	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns2_ns_set_stat, ctx, parent, &parent_stat);

	RC_WRAP(kvsal_end_transaction);
	return 0;

aborted:
	log_trace("Exit rc=%d", rc);
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

int kvsns2_access(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *ino, int flags)
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

int kvsns2_ns_get_stat(kvsns_fs_ctx_t ctx, kvsns_ino_t *ino, struct stat *bufstat)
{
	int rc;
	kvsns_inode_key_t key;
	size_t klen;

	KVSNS_DASSERT(bufstat != NULL);
	KVSNS_DASSERT(ino != NULL);

	RC_WRAP_LABEL(rc, out, _kvsns_ns_prepare_inode_key, KVSNS_KEY_STAT, *ino, &key);
	klen = rc;
	RC_WRAP_LABEL(rc, out, kvsal2_get_stat, ctx, (char *)&key, klen, bufstat);

out:
	log_trace("rc=%d", rc);
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

int kvsns2_ns_set_stat(kvsns_fs_ctx_t ctx, kvsns_ino_t *ino, struct stat *bufstat)
{
	int rc;
	kvsns_inode_key_t key;
	size_t klen;

	KVSNS_DASSERT(bufstat != NULL);
	KVSNS_DASSERT(ino != NULL);

	RC_WRAP_LABEL(rc, out, _kvsns_ns_prepare_inode_key, KVSNS_KEY_STAT, *ino, &key);
	klen = rc;
	RC_WRAP_LABEL(rc, out, kvsal2_set_stat, ctx, (char *)&key, klen, bufstat);

out:
	log_trace("rc=%d", rc);
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
