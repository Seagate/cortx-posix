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


/******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <sys/time.h>
#include <linux/limits.h>

#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

#include "kvsns_internal.h"
#include "kvsns/log.h"

/******************************************************************************/
/* Private data types */

/** Version of a namespace representation. */
typedef enum kvsns_version {
	KVSNS_VERSION_0 = 0,
	KVSNS_VERSION_INVALID,
} kvsns_version_t;

/** Key type associated with particular version of a namespace representation.
 */
typedef enum kvsns_key_type {
	KVSNS_KEY_TYPE_DIRENT = 1,
	KVSNS_KEY_TYPE_PARENT,
	KVSNS_KEY_TYPE_STAT,
	KVSNS_KEY_TYPE_INVALID,
	KVSNS_KEY_TYPES = KVSNS_KEY_TYPE_INVALID,
} kvsns_key_type_t;

static inline const char *kvsns_key_type_to_str(kvsns_key_type_t ktype)
{
	switch (ktype) {
	case KVSNS_KEY_TYPE_DIRENT:
		return "dentry";
	case KVSNS_KEY_TYPE_PARENT:
		return "parentdir";
	case KVSNS_KEY_TYPE_STAT:
		return "stat";
	case KVSNS_KEY_TYPE_INVALID:
		return "<invalid>";
	}

	KVSNS_DASSERT(0);
	return "<fail>";
}

/* Key metadata included into each key.
 * The metadata has the same size across all versions of namespace representation
 * (2 bytes).
 */
struct kvsns_key_md {
	uint8_t type;
	uint8_t version;
} __attribute__((packed));
typedef struct kvsns_key_md kvsns_key_md_t;

/* Key for parent -> child mapping.
 * Version 1: key = (parent, enrty_name), value = child.
 * NOTE: This key has variable size.
 * @see kvsns_parentdir_key for reverse mapping.
 */
struct kvsns_dentry_key {
	kvsns_ino_t  pino;
	kvsns_key_md_t md;
	kvsns_name_t name;
} __attribute((packed));

#define DENTRY_KEY_INIT(__pino, __name)			\
{							\
		.pino = __pino,				\
		.md = {					\
			.type = KVSNS_KEY_TYPE_DIRENT,	\
			.version = KVSNS_VERSION_0,	\
		},					\
		.name = __name,				\
}

#define DENTRY_KEY_PTR_INIT(key, ino, fname)	\
{							\
		key->pino = (*ino),			\
		key->md.type = KVSNS_KEY_TYPE_DIRENT,	\
		key->md.version = KVSNS_VERSION_0,	\
		key->name = *fname;			\
}

/** Dynamic size of a dentry key, i.e. the amount of bytest to be stored in
 * the KVS storage.
 */
static inline size_t kvsns_dentry_key_dsize(const struct kvsns_dentry_key *key) {
	return (sizeof(*key) - sizeof(key->name.s_str)) + key->name.s_len;
}
/** Pattern size of a dentry key, i.e. the size of a dentry prefix. */
static const size_t kvsns_dentry_key_psize =
	sizeof(struct kvsns_dentry_key) - sizeof(kvsns_name_t);


/* Key for child -> parent mapping.
 * Version 1: key = (parent, child), value = link_count, where
 * link_count is the amount of links between the parent and the child nodes.
 * @see kvsns_dentry_key for direct mapping.
 */
struct kvsns_parentdir_key {
	kvsns_ino_t ino;
	kvsns_key_md_t md;
	kvsns_ino_t pino;
} __attribute__((packed));

#define PARENTDIR_KEY_PTR_INIT(pkey, __ino, __pino)	\
{							\
		pkey->ino = *__ino,			\
		pkey->md.type = KVSNS_KEY_TYPE_DIRENT,	\
		pkey->md.version = KVSNS_VERSION_0,	\
		pkey->pino = *__pino;			\
}

#define PARENTDIR_KEY_INIT(__ino, __pino)		\
{							\
		.ino = __ino,				\
		.md = {					\
			.type = KVSNS_KEY_TYPE_PARENT,	\
			.version = KVSNS_VERSION_0,	\
		},					\
		.pino = __pino,				\
}

typedef kvsns_ino_t kvsns_dentry_val_t;

/* A generic key type for all attributes (properties) of an inode object */
struct kvsns_inode_attr_key {
	kvsns_ino_t ino;
	kvsns_key_md_t md;
} __attribute__((packed));

#define INODE_ATTR_KEY_INIT(__ino, __ktype)		\
{							\
		.ino = __ino,				\
		.md = {					\
			.type = __ktype,		\
			.version = KVSNS_VERSION_0,	\
		},					\
}
#define KVSNS_ALLOC_ARR(arr, nr) ((arr) = malloc((nr) * sizeof ((arr)[0])))
#define KVSNS_ALLOC(ptr) KVSNS_ALLOC_ARR(ptr, 1)
#define KVSNS_FREE(ptr) free(ptr)

static int kvsns_ns_get_inode_attr(kvsns_fs_ctx_t ctx,
				   const kvsns_ino_t *ino,
				   kvsns_key_type_t type,
				   void *buf, size_t buf_size);

static int kvsns_ns_set_inode_attr(kvsns_fs_ctx_t ctx,
				   const kvsns_ino_t *ino,
				   kvsns_key_type_t type,
				   const void *buf, size_t buf_size);

static int kvsns_ns_del_inode_attr(kvsns_fs_ctx_t ctx,
				   const kvsns_ino_t *ino,
				   kvsns_key_type_t type);

/******************************************************************************/
int kvsns_name_from_cstr(const char *name, kvsns_name_t *kname)
{
	int rc;

	rc = strlen(name);

	if (rc > sizeof(kname->s_str)) {
		log_err("Name '%s' is too long", name);
		rc = -EINVAL;
		goto out;
	}

	memset(kname->s_str, 0, sizeof(kname->s_str));
	memcpy(kname->s_str, name, rc);
	kname->s_len = rc;

out:
	return rc;
}

/******************************************************************************/

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

int kvsns2_update_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino, int flags)
{
	int rc;
	struct stat stat;

	KVSNS_DASSERT(ino);

	RC_WRAP_LABEL(rc, out, kvsns2_get_stat, ctx, ino, &stat);
	RC_WRAP_LABEL(rc, out, kvsns_amend_stat, &stat, flags);
	RC_WRAP_LABEL(rc, out, kvsns2_set_stat, ctx, ino, &stat);

out:
	log_trace("Update stats (%d) for %llu, rc=%d",
		  flags, *ino, rc);

	return rc;
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
	struct	stat bufstat;
	struct	timeval t;
	size_t	namelen;
	kvsns_name_t k_name;
	struct  stat parent_stat;

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

	RC_WRAP(kvsns2_next_inode, ctx, new_entry);
	RC_WRAP(kvsns2_get_stat, ctx, parent, &parent_stat);

	RC_WRAP(kvsal_begin_transaction);

	/* @todo: Alloc mero bufvecs and use it for key to avoid extra mem copy
	RC_WRAP_LABEL(rc, aborted, kvsns_alloc_dirent_key, namelen, &d_key); */

	(void) kvsns_name_from_cstr(name, &k_name);
	RC_WRAP_LABEL(rc, aborted, kvsns_tree_attach, ctx, parent, new_entry, &k_name);

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
	RC_WRAP_LABEL(rc, aborted, kvsns2_set_stat, ctx, new_entry, &bufstat);
	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns2_set_stat, ctx, parent, &parent_stat);

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

int kvsns2_get_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino,
		    struct stat *bufstat)
{
	return kvsns_ns_get_inode_attr(ctx, ino, KVSNS_KEY_TYPE_STAT,
				       bufstat, sizeof(*bufstat));
}

int kvsns2_set_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino,
		       const struct stat *bufstat)
{
	return kvsns_ns_set_inode_attr(ctx, ino, KVSNS_KEY_TYPE_STAT,
				       bufstat, sizeof(*bufstat));
}

int kvsns2_del_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino)
{
	return kvsns_ns_del_inode_attr(ctx, ino, KVSNS_KEY_TYPE_STAT);
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

/******************************************************************************/
static int kvsns_ns_get_inode_attr(kvsns_fs_ctx_t ctx,
				   const kvsns_ino_t *ino,
				   kvsns_key_type_t type,
				   void *buf, size_t buf_size)
{
	int rc;
	const struct kvsns_inode_attr_key key = INODE_ATTR_KEY_INIT(*ino, type);

	KVSNS_DASSERT(buf && ino && buf_size != 0);

	RC_WRAP_LABEL(rc, out, kvsal2_get_bin, ctx,
		      &key, sizeof(key), buf, buf_size);

out:
	log_trace("GET %llu.%s = (%d), rc=%d", *ino,
		  kvsns_key_type_to_str(type), (int) buf_size, rc);
	return rc;
}

static int kvsns_ns_set_inode_attr(kvsns_fs_ctx_t ctx,
				   const kvsns_ino_t *ino,
				   kvsns_key_type_t type,
				   const void *buf, size_t buf_size)
{
	int rc;
	const struct kvsns_inode_attr_key key = INODE_ATTR_KEY_INIT(*ino, type);

	KVSNS_DASSERT(buf && ino && buf_size != 0);

	RC_WRAP_LABEL(rc, out, kvsal2_set_bin, ctx,
		      &key, sizeof(key), buf, buf_size);

out:
	log_trace("SET %llu.%s = (%d), rc=%d", *ino,
		  kvsns_key_type_to_str(type), (int) buf_size, rc);
	return rc;

}

static int kvsns_ns_del_inode_attr(kvsns_fs_ctx_t ctx,
				   const kvsns_ino_t *ino,
				   kvsns_key_type_t type)
{
	int rc;
	const struct kvsns_inode_attr_key key = INODE_ATTR_KEY_INIT(*ino, type);

	KVSNS_DASSERT(ino);

	RC_WRAP_LABEL(rc, out, kvsal2_del_bin, ctx, &key, sizeof(key));

out:
	log_trace("DEL %llu.%s, rc=%d", *ino,
		  kvsns_key_type_to_str(type), rc);
	return rc;
}

/******************************************************************************/


int kvsns_get_inode(kvsns_fs_ctx_t ctx, kvsns_ino_t *ino, struct stat *bufstat)
{
	int rc;
	const struct kvsns_inode_attr_key key =
		INODE_ATTR_KEY_INIT(*ino, KVSNS_KEY_TYPE_STAT);

	KVSNS_DASSERT(bufstat != NULL);
	KVSNS_DASSERT(ino != NULL);

	RC_WRAP_LABEL(rc, out, kvsal2_get_bin, ctx,
		      &key, sizeof(key), bufstat, sizeof(*bufstat));

out:
	log_trace("GET %llu.stat, rc=%d", *ino, rc);
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

/******************************************************************************/
int kvsns_tree_detach(kvsns_fs_ctx_t fs_ctx,
		      const kvsns_ino_t *parent_ino,
		      const kvsns_ino_t *ino,
		      const kvsns_name_t *node_name)
{
	int rc;
	const struct kvsns_dentry_key dentry_key =
		DENTRY_KEY_INIT(*parent_ino, *node_name);
	const struct kvsns_parentdir_key parent_key =
		PARENTDIR_KEY_INIT(*ino, *parent_ino);
	uint64_t parent_value;

	// Remove dentry
	RC_WRAP_LABEL(rc, out, kvsal2_del_bin, fs_ctx,
		      &dentry_key, kvsns_dentry_key_dsize(&dentry_key));

	// Update parent link count
	RC_WRAP_LABEL(rc, out, kvsal2_get_bin, fs_ctx,
		      &parent_key, sizeof(parent_key),
		      &parent_value, sizeof(parent_value));
	parent_value--;
	if (parent_value > 0){
		RC_WRAP_LABEL(rc, out, kvsal2_set_bin, fs_ctx,
			      &parent_key, sizeof(parent_key),
			      &parent_value, sizeof(parent_value));
	} else {
		RC_WRAP_LABEL(rc, out, kvsal2_del_bin, fs_ctx,
			      &parent_key, sizeof(parent_key));
	}

	// Update stats
	RC_WRAP_LABEL(rc, out, kvsns2_update_stat, fs_ctx, parent_ino,
		      STAT_CTIME_SET|STAT_INCR_LINK);

out:
	log_debug("tree_detach(%p,pino=%llu,ino=%llu,n=%.*s) = %d",
		  fs_ctx, *parent_ino, *ino, node_name->s_len, node_name->s_str, rc);
	return rc;
}

/******************************************************************************/
int kvsns_tree_attach(kvsns_fs_ctx_t fs_ctx,
		      const kvsns_ino_t *parent_ino,
		      const kvsns_ino_t *ino,
		      const kvsns_name_t *node_name)
{
	int rc;
	struct kvsns_dentry_key *dentry_key;
	struct kvsns_parentdir_key *parent_key;

	KVSNS_ALLOC(dentry_key);
	if (dentry_key == NULL) {
		rc = -ENOMEM;
		log_err("dentry_key alloc failed!");
		goto out;
	}
	/* @todo rename this to DENTRY_KEY_INIT once all the instances of
	 * DENTRY_KEY_INIT are replaced with DENTRY_KEY_PTR_INIT */

	DENTRY_KEY_PTR_INIT(dentry_key, parent_ino, node_name);
	kvsns_ino_t dentry_value = *ino;
	uint64_t parent_value;
	uint64_t val_size = sizeof parent_value;
	uint64_t *parent_val_ptr = NULL;

	// Add dentry
	RC_WRAP_LABEL(rc, free_dentrykey, kvsal3_set_bin, fs_ctx,
		      dentry_key, kvsns_dentry_key_dsize(dentry_key),
		      &dentry_value, sizeof(dentry_value));

	KVSNS_ALLOC(parent_key);
	if (parent_key == NULL) {
		rc = -ENOMEM;
		log_err("parent_key alloc_failed!");
		goto free_dentrykey;
	}

	/* @todo rename this to PARENT_KEY_INIT once all the instances of
	 * PARENT_KEY_INIT are replaced with PARENT_DIR_KEY_PTR_INIT */
	PARENTDIR_KEY_PTR_INIT(parent_key, ino, parent_ino);
	// Update parent link count
	rc = kvsal3_get_bin(fs_ctx,
			    parent_key, sizeof(parent_key),
			    (void **)&parent_val_ptr, &val_size);
	if (rc == -ENOENT) {
		parent_value = 0;
		rc = 0;
	}

	if (rc < 0) {
		log_err("Failed to get parent key for %llu/%llu", *parent_ino,
			 *ino);
		goto free_parentkey;
	}

	if (parent_val_ptr != NULL)
		parent_value = *parent_val_ptr;

	parent_value++;
	RC_WRAP_LABEL(rc, out, kvsal3_set_bin, fs_ctx,
		      parent_key, sizeof(parent_key),
		      (void **)&parent_value, sizeof(parent_value));

	// Update stats
	RC_WRAP_LABEL(rc, free_parentkey, kvsns2_update_stat, fs_ctx, parent_ino,
		      STAT_CTIME_SET|STAT_INCR_LINK);

free_parentkey:
	   KVSNS_FREE(parent_key);

free_dentrykey:
	   KVSNS_FREE(dentry_key);
out:
	log_debug("tree_attach(%p,pino=%llu,ino=%llu,n=%.*s) = %d",
		  fs_ctx, *parent_ino, *ino, node_name->s_len, node_name->s_str, rc);
	return rc;
}

/******************************************************************************/
int kvsns_tree_rename_link(kvsns_fs_ctx_t fs_ctx,
			   const kvsns_ino_t *parent_ino,
			   const kvsns_ino_t *ino,
			   const kvsns_name_t *old_name,
			   const kvsns_name_t *new_name)
{
	int rc;
	struct kvsns_dentry_key dentry_key =
		DENTRY_KEY_INIT(*parent_ino, *old_name);
	const kvsns_ino_t dentry_value = *ino;

	// The caller must ensure that the entry exists prior renaming */
	KVSNS_DASSERT(kvsns_tree_lookup(fs_ctx, parent_ino, old_name, NULL) == 0);

	// Remove dentry
	RC_WRAP_LABEL(rc, out, kvsal2_del_bin, fs_ctx,
		      &dentry_key, kvsns_dentry_key_dsize(&dentry_key));

	dentry_key.name = *new_name;

	// Add detntry
	RC_WRAP_LABEL(rc, out, kvsal2_set_bin, fs_ctx,
		      &dentry_key, kvsns_dentry_key_dsize(&dentry_key),
		      &dentry_value, sizeof(dentry_value));

	// Update ctime stat
	RC_WRAP_LABEL(rc, out, kvsns2_update_stat, fs_ctx, parent_ino,
		      STAT_CTIME_SET);
out:
	log_debug("tree_rename(%p,pino=%llu,ino=%llu,o=%.*s,n=%.*s) = %d",
		  fs_ctx, *parent_ino, *ino,
		  old_name->s_len, old_name->s_str,
		  new_name->s_len, new_name->s_str, rc);
	return rc;
}

/******************************************************************************/
int kvsns_tree_has_children(kvsns_fs_ctx_t fs_ctx,
			    const kvsns_ino_t *ino,
			    bool *has_children)
{
	int rc;
	const struct kvsns_dentry_key key = DENTRY_KEY_INIT(*ino, {});

	RC_WRAP_LABEL(rc, out, kvsal_key_prefix_exists, fs_ctx,
		      &key, kvsns_dentry_key_psize, has_children);

out:
	log_debug("%llu has at least %d children, rc=%d",
		  *ino, (int) *has_children, rc);
	return rc;
}

/******************************************************************************/
int kvsns_tree_lookup(kvsns_fs_ctx_t fs_ctx,
		      const kvsns_ino_t *parent_ino,
		      const kvsns_name_t *name,
		      kvsns_ino_t *ino)
{
	struct kvsns_dentry_key *dkey = NULL;
	kvsns_ino_t value = 0;
	int rc;
	uint64_t val_size = sizeof value;
	uint64_t *val_ptr = NULL;

	KVSNS_DASSERT(parent_ino && name);
	KVSNS_ALLOC(dkey);
	if (dkey == NULL) {
		log_err("dentry key alloc failed!");
		rc = -ENOMEM;
		goto out;
	}
	DENTRY_KEY_PTR_INIT(dkey, parent_ino, name);

	RC_WRAP_LABEL(rc, cleanup, kvsal3_get_bin, fs_ctx,
		      dkey, kvsns_dentry_key_dsize(dkey),
		      (void **)&val_ptr, &val_size);

	if (ino) {
		KVSNS_DASSERT(val_ptr != NULL);
		*ino = *val_ptr;
		value = *ino;
	}
cleanup:
	KVSNS_FREE(dkey);

out:
	log_debug("GET %llu.dentries.%.*s=%llu, rc=%d",
		  *parent_ino, name->s_len, name->s_str, value, rc);
	return rc;
}

/******************************************************************************/
