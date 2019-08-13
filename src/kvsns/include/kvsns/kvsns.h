/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL  philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_test.c
 * KVSNS: simple test
 */

#ifndef _KVSNS_H
#define _KVSNS_H

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/xattr.h>

#include <kvsns/common.h>
#include <kvsns/kvsal.h>
#include <kvsns/log.h>

#define KVSNS_DASSERT(cond) assert(cond)

#define KVSNS_ROOT_INODE 2LL
#define KVSNS_ROOT_UID 0

#define KVSNS_DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"
#define KVSNS_STORE "KVSNS_STORE"
#define KVSNS_SERVER "KVSNS_SERVER"
#define KLEN 256
#define VLEN 256

#define KVSNS_URL "kvsns:"
#define KVSNS_URL_LEN 6

#define STAT_MODE_SET	0x001
#define STAT_UID_SET	0x002
#define STAT_GID_SET	0x004
#define STAT_SIZE_SET	0x008
#define STAT_ATIME_SET	0x010
#define STAT_MTIME_SET	0x020
#define STAT_CTIME_SET	0x040
#define STAT_INCR_LINK  0x080
#define STAT_DECR_LINK  0x100
#define STAT_SIZE_ATTACH  0x200

#define STAT_OWNER_READ		0400	/* Read by owner. */
#define STAT_OWNER_WRITE	0200	/* Write by owner. */
#define STAT_OWNER_EXEC		0100	/* Execute by owner. */
#define STAT_GROUP_READ		0040	/* Read by group. */
#define STAT_GROUP_WRITE	0020	/* Write by group. */
#define STAT_GROUP_EXEC		0010	/* Execute by group. */
#define STAT_OTHER_READ		0004	/* Read by other. */
#define STAT_OTHER_WRITE	0002	/* Write by other. */
#define STAT_OTHER_EXEC		0001	/* Execute by other. */

#define KVSNS_ACCESS_READ	1
#define KVSNS_ACCESS_WRITE	2
#define KVSNS_ACCESS_EXEC	4

/* If you see this define it means that this code is intentionaly left under ifdef
 * as an example of open() implementaion if case if we need to support storing
 * file states inside KVS. This code won't work if enabled, but this is
 * a base for possible implementation of "non-volatile" open.
 * The parts of code wrapped with this define
 * can be removed only when KVSFS fully supports multi-client open4
 * and all types of locks, and backup engine is implemented for KVSNS
 * (or if the problem of server restart has been resolved in a different way).
 * For code reviewers: I'd prefer to get rid of this code because we still
 * have the history in the VCS and the open source version of this code.
 * Please, let me know if you agree with that.
 */
// #define KVSNS_ENABLE_KVS_OPEN


//#define KVSNS_ENABLE_NO_INDEX_API

/** Access level required to create an object in a directory
 * (in other words, to link an inode into a directory).
 */
#define KVSNS_ACCESS_CREATE_ENTITY \
	(KVSNS_ACCESS_WRITE | KVSNS_ACCESS_EXEC)
/** Access level required to delete an object in a directory
 * (i.e., to unlink an inode from it).
 */
#define KVSNS_ACCESS_DELETE_ENTITY \
	(KVSNS_ACCESS_WRITE | KVSNS_ACCESS_EXEC)

/** Access level required to list objecs in a directory (READDIR). */
#define KVSNS_ACCESS_LIST_DIR \
	(KVSNS_ACCESS_EXEC)

#define KVSNS_NULL_FS_CTX NULL
#define KVSNS_FS_ID_DEFAULT 0

/* KVSAL related definitions and functions */
/* TODO: replace with a struct */
typedef unsigned long long int kvsns_ino_t;
/* TODO: replace with a struct */
typedef unsigned long int kvsns_fsid_t;
/* TODO: replace with an opaque struct or just a struct at least */
typedef void *kvsns_fs_ctx_t;

/* KVSNS related definitions and functions */
typedef struct kvsns_cred__ {
	uid_t uid;
	gid_t gid;
} kvsns_cred_t;

typedef struct kvsns_dentry_ {
	char name[NAME_MAX];
	kvsns_ino_t inode;
	struct stat stats;
} kvsns_dentry_t;

typedef struct kvsns_open_owner_ {
	int pid;
	int tid;
} kvsns_open_owner_t;

typedef struct kvsns_file_open_ {
	kvsns_ino_t ino;
	kvsns_open_owner_t owner;
	int flags;
} kvsns_file_open_t;

enum kvsns_type {
	KVSNS_DIR = 1,
	KVSNS_FILE = 2,
	KVSNS_SYMLINK = 3
};

typedef struct kvsns_xattr__ {
	char name[NAME_MAX];
} kvsns_xattr_t;

typedef struct kvsns_fid {
	uint64_t f_hi;
	uint64_t f_lo;
} kvsns_fid_t;

static inline int prepare_key(char k[], size_t klen, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));
/**
 * Generates a key based on variable parameter list.
 *
 * @param: k - Buffer to store the generated key
 * @param: klen - Key length of the buffer
 * @param: fmt - String that contains a format string
 * @return buffer length ( > 1) including '\0' if successful,
 *	   otherwise, -errno.
 */
static inline int prepare_key(char k[], size_t klen, const char *fmt, ...)
{
	int rc;
	va_list args;

	va_start(args, fmt);
	rc = vsnprintf(k, klen, fmt, args);
	va_end(args);

	if (rc < 0) {
		rc = -errno;
		log_err("Failed to format a key (fmt='%s', errno=%d)",
			fmt, rc);
		goto out;
	}

	KVSNS_DASSERT(rc != 0);
	KVSNS_DASSERT(rc < klen);

	rc = rc + 1 /* For \0 */;
	log_debug("key=%s len=%d", k, rc);

out:
	return rc;
}

/**
 * Start the kvsns library. This should be done by every thread using the library
 *
 * @note: this function will allocate required resources and set useful
 * variables to their initial value. As the programs ends kvsns_stop() should be
 * invoked to perform all needed cleanups.
 * In this version of the API, it takes no parameter, but this may change in
 * later versions.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_start(const char *config);

/**
 * Stops the kvsns library. This should be done by every thread using the library
 *
 * @note: this function frees what kvsns_start() allocates.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_stop(void);


/**
 * Create the root of the namespace.
 *
 * @param openbar: if true, everyone can access the root directory
 * if false, only root can.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_init_root(int openbar);

/**
 * Check is a given user can access an inode
 *
 * @note: this call is similar to POSIX's access() call. It behaves the same.
 *
 * @param cred - pointer to user's credentials
 * @param ino - pointer to inode whose access is to be checked.
 * @params flags - access to be tested. The flags are the same as those used
 * by libc's access() function.
 *
 * @return 0 if access is granted, a negative value means an error. -EPERM
 * is returned when access is not granted
 */
int kvsns_access(kvsns_cred_t *cred, kvsns_ino_t *ino, int flags);

/**
 * Check is a given user can access an inode.
 *
 * @note: this call is similar to POSIX's access() call. It behaves the same.
 *
 * @param ctx - File system context
 * @param cred - pointer to user's credentials
 * @param ino - pointer to inode whose access is to be checked.
 * @params flags - access to be tested. The flags are the same as those used
 * by libc's access() function.
 *
 * @return 0 if access is granted, a negative value means an error. -EPERM
 * is returned when access is not granted
 */
int kvsns2_access(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *ino, int flags);

/**
 * Creates a file.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the file to be created
 * @param mode - Unix mode for the new entry
 * @paran newino - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_creat(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		char *name, mode_t mode, kvsns_ino_t *newfile);


/* Atomic file creation.
 * The functions create a new file, sets new attributes, gets them back
 * to the caller. That's all must be done within a transaction because we
 * cannot leave the file in the storage if we cannot set/get its stats.
 * TODO: This operations is not atomic yet but will be eventually.
 * @param[in] stat_in - New stats to be set.
 * @param[in] stat_in_flags - Defines which stat values must be set.
 * @param[out] stat_out - Final stat values of the created file.
 * @param[out] newfile - The inode number of the created file.
 */
int kvsns_creat_ex(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		   char *name, mode_t mode, struct stat *stat_in,
		   int stat_in_flags, kvsns_ino_t *newfile,
		   struct stat *stat_out);
/**
 * Creates a directory.
 *
 * @param ctx - pointer to filesystem context
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be created
 * @param mode - Unix mode for the new entry
 * @paran newino - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_mkdir(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 mode_t mode, kvsns_ino_t *newdir);

/**
 * Creates a symbolic link
 *
 * @param fs_ctx - A context associated with the filesystem.
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be created
 * @param content - the content of the symbolic link to be created
 * @paran newlnk - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_symlink(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  char *content, kvsns_ino_t *newlnk);

/**
 * Reads the content of a symbolic link
 *
 * @param fs_ctx - A context associated with the filesystem.
 * @param cred - pointer to user's credentials
 * @param link - pointer to the symlink's inode
 * @param content - [OUT] buffer containing the read content.
 * @param size - [OUT] size read by this call
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_readlink(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *link,
		   char *content, size_t *size);

/**
 * Removes a directory. It won't be deleted if not empty.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be remove.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name);

/**
 * Removes a directory. It won't be deleted if not empty.
 * @param ctx - A context associated with a filesystem
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be remove.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_rmdir(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		 char *name);

/**
 * Removes a file or a symbolic link.
 *
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the entry to be remove.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);

/**
 * Creates a file's hardlink
 *
 * @note: this call will failed if not performed on a file.
 *
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode.
 * @param dino - pointer to destination directory's inode
 * @param dname - name of the new entry in dino
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_unlink(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);

/**
 * Creates a file's hardlink
 *
 * @note: this call will failed if not performed on a file.
 *
 * @param fs_ctx - A context associated with the filesystem.
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode.
 * @param dino - pointer to destination directory's inode
 * @param dname - name of the new entry in dino
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_link(kvsns_fs_ctx_t fs_ctx, kvsns_cred_t *cred, kvsns_ino_t *ino, kvsns_ino_t *dino,
	       char *dname);

/**
 * Renames an entry in a filesystem.
 * @param[in] fs_ctx - A context associated with the filesystem.
 * @param[in] cred - User's credentials.
 * @param [in] sino - Inode of the source dir.
 * @param[in] sname - Name of the entry within `sino`.
 * @param[in] dino - Inode of the destination dir.
 * @param[in] dname - Name of the entry within `dino`.
 * @return 0 if successfull, oterwise -errno.
 */
int kvsns_rename(kvsns_fs_ctx_t fs_ctx, kvsns_cred_t *cred,
		 kvsns_ino_t *sino_dir, char *sname,
		 kvsns_ino_t *dino_dir, char *dname);

/**
 * Finds the inode of an entry whose parent and name are known. This is the
 * basic "lookup" operation every filesystem implements.
 *
 * @param ctx - Filesystem context
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the entry to be found.
 * @paran myino - [OUT] points to the found ino if successful.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_lookup(void *ctx, kvsns_cred_t *cred,
		  kvsns_ino_t *parent, char *name,
		  kvsns_ino_t *myino);
/**
 * Finds the parent inode of an entry.
 *
 * @note : because of this call, directories do not contain explicit
 * "." and ".." directories. The namespace's root directory is the only
 * directory to be its own parent directory.
 *
 * @param cred - pointer to user's credentials
 * @param where - pointer to current inode
 * @param name - name of the entry to be found.
 * @paran parent - [OUT] points to parent directory's inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *where, kvsns_ino_t *parent);

/**
 * Finds the root of the namespace
 *
 * @param ino - [OUT] points to root inode if successful
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
static inline
int kvsns_get_root(kvsns_ino_t *ino)
{
	*ino = KVSNS_ROOT_INODE;
	return 0;
}

/**
 * Gets attributes for a known inode.
 *
 * @note: the call is similar to stat() call in libc. It uses the structure
 * "struct stat" defined in the libC.
 * @param ctx - Filesystem context
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode
 * @param buffstat - [OUT] points to inode's stats.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_getattr(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		   struct stat *buffstat);

/**
 * Sets attributes for a known inode.
 *
 * This call uses a struct stat structure as input. This structure will
 * contain the values to be set. More than one can be set in a single call.
 * The parameter "statflags: indicates which fields are to be considered:
 *  STAT_MODE_SET: sets mode
 *  STAT_UID_SET: sets owner
 *  STAT_GID_SET: set group owner
 *  STAT_SIZE_SET: set size (aka truncate)
 *  STAT_ATIME_SET: sets atime
 *  STAT_MTIME_SET: sets mtime
 *  STAT_CTIME_SET: set ctime
 *
 * @param ctx - Filesystem context
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode
 * @param setstat - a stat structure containing the new values
 * @param statflags - a bitmap that tells which attributes are to be set
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_setattr(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		   struct stat *setstat, int statflags);

/* TODO:Unknown features: Implement statvfs-like call here */

/** A callback to be used in kvsns_readddir.
 * @retval true continue iteration.
 * @retval false stop iteration.
 */
typedef bool (*kvsns_readdir_cb_t)(void *ctx, const char *name,
			    const kvsns_ino_t *ino);

/** Walk over a directory "dir_ino" and call cb(cb_ctx, entry_name, entry_ino)
 * for each dentry.
 */
int kvsns_readdir(kvsns_fs_ctx_t fs_ctx,
		  const kvsns_cred_t *cred,
		  const kvsns_ino_t *dir_ino,
		  kvsns_readdir_cb_t cb,
		  void *cb_ctx);

/**
 * Opens a file for reading and/or writing
 *
 * @note: this call use the same flags as LibC's open() call. You must know
 * the inode to call this function so you can't use it for creating a file.
 * In this case, kvsns_create() is to be invoked.
 *
 * @todo: mode parameter is unused. Remove it.
 *
 * @param ctx - filesystem context pointer
 * @param cred - pointer to user's credentials
 * @param ino - file's inode
 * @param flags - open flags (see man 2 open)
 * @param mode - unused
 * @param fd - [OUT] handle to opened file.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_open(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		int flags, mode_t mode, kvsns_file_open_t *fd);

/**
 * Opens a file by name and parent directory
 *
 * @note: this call use the same flags as LibC's openat() call. You must know
 * the inode to call this function so you can't use it for creating a file.
 * In this case, kvsns_create() is to be invoked.
 *
 * @todo: mode parameter is unused. Remove it.
 *
 * @param ctx - filesystem context pointer
 * @param cred - pointer to user's credentials
 * @param parent - parent's inode
 * @param name- file's name
 * @param flags - open flags (see man 2 open)
 * @param mode - unused
 * @param fd - [OUT] handle to opened file.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_openat(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		  int flags, mode_t mode, kvsns_file_open_t *fd);

/**
 * Closes a file descriptor
 *
 * @param ctx - filesystem context pointer
 * @param fd - handle to opened file
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns2_close(void *ctx, kvsns_file_open_t *fd);

/* A placeholder for preserving the openowner logic.
 * It does nothing, just returns False.
 */
int kvsns_is_open(kvsns_fs_ctx_t *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		  bool *is_open);

/**
 * Writes data to an opened fd
 *
 * @param cred - pointer to user's credentials
 * @param fd - handle to opened file
 * @param buf - data to be written
 * @param count - size of buffer to be written
 * @param offset - write offset
 *
 * @return written size or a negative "-errno" in case of failure
 */
ssize_t kvsns_write(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		  void *buf, size_t count, off_t offset);

/**
 * Writes data to an opened fd
 *
 * @param ctx - filesystem context pointer
 * @param cred - pointer to user's credentials
 * @param fd - handle to opened file
 * @param buf - write data
 * @param count - size of buffer to be read
 * @param offset - write offset
 *
 * @return write size or a negative "-errno" in case of failure
 */
ssize_t kvsns2_write(void *ctx, kvsns_cred_t *cred, kvsns_file_open_t *fd,
		     void *buf, size_t count, off_t offset);

/**
 * Reads data from an opened fd
 *
 * @param cred - pointer to user's credentials
 * @param fd - handle to opened file
 * @param buf - [OUT] read data
 * @param count - size of buffer to be read
 * @param offset - read offset
 *
 * @return read size or a negative "-errno" in case of failure
 */
ssize_t kvsns_read(kvsns_cred_t *cred, kvsns_file_open_t *fd,
		  void *buf, size_t count, off_t offset);

/**
 * Reads data from an opened fd
 *
 * @param ctx - filesystem context pointer
 * @param cred - pointer to user's credentials
 * @param fd - handle to opened file
 * @param buf - [OUT] read data
 * @param count - size of buffer to be read
 * @param offset - read offset
 *
 * @return read size or a negative "-errno" in case of failure
 */
ssize_t kvsns2_read(void *ctx, kvsns_cred_t *cred, kvsns_file_open_t *fd,
		    void *buf, size_t count, off_t offset);

/* Xattr */

/**
 * Sets an xattr to an entry
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - xattr's name
 * @param value - buffer with xattr's value
 * @param size - size of previous buffer
 * @param flags - overwrite behavior, see "man 2 setxattr". Overwitte is
 * prohibited if set to XATTR_CREATE
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_setxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		  char *name, char *value, size_t size, int flags);

/**
 * Gets an xattr to an entry
 *
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - xattr's name
 * @param value - [OUT] buffer with xattr's value
 * @param size - [OUT] size of previous buffer
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		  char *name, char *value, size_t *size);

/**
 * Lists an entry's xattr
 *
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @paran offset - read offset
 * @param list - [OUT] list of found xattrs
 * @param size - [INOUT] In: size of list, Out: size of read list.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset,
		 kvsns_xattr_t *list, int *size);

/**
 * Removes an xattr
 *
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - name of xattr to be removed
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name);

/**
 * Removes all xattr for an inode
 *
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_remove_all_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino);

/* For utility */

/**
 * Deletes everything in the namespace but the root directory...
 *
 * @param (node) - void function.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_mr_proper(void);


/**
 *  High level API: copy a file from the KVSNS to a POSIX fd
 *
 * @param cred - pointer to user's credentials
 * @param kfd  - pointer to kvsns's open fd
 * @param fd_dest - POSIX fd to copy file into
 * @param iolen -recommend IO size
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_cp_from(kvsns_cred_t *cred, kvsns_file_open_t *kfd,
		  int fd_dest, int iolen);

/**
 *  High level API: copy a file to the KVSNS from a POSIX fd
 *
 * @param cred - pointer to user's credentials
 * @param fd_dest - POSIX fd to retrieve data from
 * @param kfd  - pointer to kvsns's open fd
 * @param iolen -recommend IO size
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_cp_to(kvsns_cred_t *cred, int fd_source,
		kvsns_file_open_t *kfd, int iolen);

/**
 * High level API: Get a fs specific handle for the passed fs id.
 * @param fs_id Filesystem id
 * @param fs_ctx [OUT] File system ctx for the passed fs id.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_fsid_to_ctx(kvsns_fsid_t fs_id, kvsns_fs_ctx_t *fs_ctx);

/**
 * High level API: Create a fs context for the passed fs id. This function
 * would be invoked during kvsns_start.
 * @todo: Currently only sets a global context to fs_ctx.
 * @param fs_id Filesystem id
 * @param fs_ctx: Generated fs_ctx for that fs_id
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsns_create_fs_ctx(kvsns_fsid_t fs_id, kvsns_fs_ctx_t *fs_ctx);

#endif
