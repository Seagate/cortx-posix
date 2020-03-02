/*
 * Filename:         efs.h
 * Description:      EOS file system interfaces

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system interfaces.
*/

#ifndef _EFS_H
#define _EFS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <utils.h>
#include <sys/stat.h>
#include <str.h> /* str256_t */
#include <object.h> /* obj_id_t */
#include "namespace.h"

/* forword declations */
struct kvs_idx;

/**
 * Start the efs library. This should be done by every thread using the library
 *
 * @note: this function will allocate required resources and set useful
 * variables to their initial value. As the programs ends efs_init() should be
 * invoked to perform all needed cleanups.
 *
 * @param: const char *config.
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_init(const char *config);

int efs_fs_init(void);
/**
 * Finalizes the efs library.
 * This should be done by every thread using the library
 *
 * @note: this function frees what efs_init() allocates.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_fini(void);

int efs_fs_fini(void);
/**
 * @todo : This is s/efs_fs_ctx_t/efs_fs_ctx_t
 * We need to comeup with proper efs_fs_ctx_t object.
 */
typedef void *efs_ctx_t;

/**
 * @todo : When efs_ctx_t is defined, EFS_NULL_FS_CTX,
 * will also be redefined.
 */
#define EFS_NULL_FS_CTX (NULL)

#define EFS_DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

#define EFS_ROOT_INODE 2LL
#define EFS_ROOT_UID 0

typedef unsigned long long int efs_ino_t;

#define STAT_MODE_SET   0x001
#define STAT_UID_SET    0x002
#define STAT_GID_SET    0x004
#define STAT_SIZE_SET   0x008
#define STAT_ATIME_SET  0x010
#define STAT_MTIME_SET  0x020
#define STAT_CTIME_SET  0x040
#define STAT_INCR_LINK  0x080
#define STAT_DECR_LINK  0x100
#define STAT_SIZE_ATTACH  0x200

#define STAT_OWNER_READ         0400    /* Read by owner. */
#define STAT_OWNER_WRITE        0200    /* Write by owner. */
#define STAT_OWNER_EXEC         0100    /* Execute by owner. */
#define STAT_GROUP_READ         0040    /* Read by group. */
#define STAT_GROUP_WRITE        0020    /* Write by group. */
#define STAT_GROUP_EXEC         0010    /* Execute by group. */
#define STAT_OTHER_READ         0004    /* Read by other. */
#define STAT_OTHER_WRITE        0002    /* Write by other. */
#define STAT_OTHER_EXEC         0001    /* Execute by other. */

#define EFS_ACCESS_READ       1
#define EFS_ACCESS_WRITE      2
#define EFS_ACCESS_EXEC       4
#define EFS_ACCESS_SETATTR    8

/** Access level required to create an object in a directory
 * (in other words, to link an inode into a directory).
 */
#define EFS_ACCESS_CREATE_ENTITY \
	(EFS_ACCESS_WRITE | EFS_ACCESS_EXEC)

/** Access level required to delete an object in a directory
 * (i.e., to unlink an inode from it).
 */
#define EFS_ACCESS_DELETE_ENTITY \
	(EFS_ACCESS_WRITE | EFS_ACCESS_EXEC)

/** Access level required to list objecs in a directory (READDIR). */
#define EFS_ACCESS_LIST_DIR \
	(EFS_ACCESS_EXEC)

enum efs_file_type {
	EFS_FT_DIR = 1,
	EFS_FT_FILE = 2,
	EFS_FT_SYMLINK = 3
};

typedef struct efs_cred__ {
        uid_t uid;
        gid_t gid;
} efs_cred_t;

/**
 * Create the root of the namespace.
 *
 * @param index - File system index.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_tree_create_root(struct kvs_idx *index);

/**
 * Delete the root of the namespace.
 *
 * @param index - File system index.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_tree_delete_root(struct kvs_idx *index);

int efs_access_check(const efs_cred_t *cred, const struct stat *stat,
                     int flags);

typedef obj_id_t efs_fid_t;

typedef void *efs_fs_ctx_t;
/* @todo:
 * All below APIs need to be moved to internal headers
 * once all migration of efs completes.
 */
/* Inode Attributes API */
int efs_get_stat(efs_ctx_t ctx, const efs_ino_t *ino,
		 struct stat **bufstat);
int efs_set_stat(efs_fs_ctx_t ctx, const efs_ino_t *ino,
		 struct stat *bufstat);
int efs_del_stat(efs_fs_ctx_t ctx, const efs_ino_t *ino);
int efs_update_stat(efs_fs_ctx_t ctx, const efs_ino_t *ini, int flags);
int efs_get_symlink(efs_fs_ctx_t ctx, const efs_ino_t *ino,
		    void **buf, size_t *buf_size);
int efs_set_symlink(efs_fs_ctx_t ctx, const efs_ino_t *ino,
		    void *buf, size_t buf_size);
int efs_del_symlink(efs_fs_ctx_t ctx, const efs_ino_t *ino);
int efs_amend_stat(struct stat *stat, int flags);

int efs_create_entry(efs_ctx_t *ctx, efs_cred_t *cred, efs_ino_t *parent,
                     char *name, char *lnk, mode_t mode,
                     efs_ino_t *new_entry, enum efs_file_type type);

/******************************************************************************/
/* Tree API:
 *	- Attach/Detach an inode to/from another inode.
 *	- Get list of child and parent inodes.
 *	- Look up a parent/child of an inode.
 */

/******************************************************************************/
/** Removes an existing link between a parent node and a child node.
 * @param[in] ctx - Filesystem context
 * @param[in] parent_ino - Inode of the parent
 * @param[in] ino - Inode of the child
 * @param[in] node_name - Name of the entry in the `parent_ino` which is
 * associated with the given `ino`.
 * @return 0 if successful, otherwise -errno.
 */
int efs_tree_detach(efs_fs_ctx_t fs_ctx,
		    const efs_ino_t *parent_ino,
		    const efs_ino_t *ino,
		    const str256_t *node_name);

/******************************************************************************/
/** Create a new link between two nodes in a filesysytem.
 * @param[in] ctx - Filesystem context
 * @param[in] parent_ino - Inode of the parent
 * @param[in] ino - Inode of the child
 * @param[in] node_name - Name of a new entry in the `parent_ino` which will be
 * associated with the given `ino`.
 * @return 0 if successful, otherwise -errno.
 */
int efs_tree_attach(efs_fs_ctx_t ctx,
		    const efs_ino_t *parent_ino,
		    const efs_ino_t *ino,
		    const str256_t *node_name);


/******************************************************************************/
/** Change the name of a link between a parent node and a child node without
 * modifying the link itself.
 */
int efs_tree_rename_link(efs_fs_ctx_t fs_ctx,
			 const efs_ino_t *parent_ino,
			 const efs_ino_t *ino,
			 const str256_t *old_name,
			 const str256_t *new_name);

/******************************************************************************/
/** Check if the given inode has at least one child inode linked into it. */
int efs_tree_has_children(efs_fs_ctx_t fs_ctx,
			  const efs_ino_t *ino,
			  bool *has_children);

/******************************************************************************/
/* Get the inode of an object linked into a parent inode.
 * @param[in] parent_ino - Inode of the parent object.
 * @param[in] name - Name of the object to be found.
 * @param[out, optional] ino - Inode of the found object.
 * @return 0 if successfull, otherwise -errno, including -ENOENT
 * if the object does not exist.
 */
int efs_tree_lookup(efs_fs_ctx_t fs_ctx,
		    const efs_ino_t *parent_ino,
		    const str256_t *name,
		    efs_ino_t *ino);

/** A callback to be used in efs_readddir.
 * @retval true continue iteration.
 * @retval false stop iteration.
 */
typedef bool (*efs_readdir_cb_t)(void *ctx, const char *name,
				 const efs_ino_t *ino);

/******************************************************************************/
/** Walk over children (dentries) if an inode (directory). */
int efs_tree_iter_children(efs_fs_ctx_t fs_ctx,
			   const efs_ino_t *ino,
			   efs_readdir_cb_t cb,
			   void *cb_ctx);

/******************************************************************************/
/* EFS internal data types */

/** Version of a efs representation. */
typedef enum efs_version {
	EFS_VERSION_0 = 0,
	EFS_VERSION_INVALID,
} efs_version_t;

/** Key type associated with particular version of a namespace representation.
 */
typedef enum efs_key_type {
	EFS_KEY_TYPE_DIRENT = 1,
	EFS_KEY_TYPE_PARENT,
	EFS_KEY_TYPE_STAT,
	EFS_KEY_TYPE_SYMLINK,
	EFS_KEY_TYPE_INODE_KFID,
	EFS_KEY_TYPE_GI_INDEX,
        EFS_KEY_TYPE_FS_ID_FID,
        EFS_KEY_TYPE_FS_NAME,
        EFS_KEY_TYPE_FS_ID,
	EFS_KEY_TYPE_FS_ID_NEXT,
	EFS_KEY_TYPE_INO_NUM_GEN,
	EFS_KEY_TYPE_INVALID,
} efs_key_type_t;

typedef unsigned long int efs_fsid_t;

typedef struct efs_inode_attr_key efs_inode_kfid_key_t;

struct efs_fs_attr {
        efs_fsid_t fs_id;
        efs_fid_t fid;
        str256_t fs_name;
};

/* Key metadata included into each key.
 * The metadata has the same size across all versions of EFS object key representation
 * (2 bytes).
 */
struct efs_key_md {
	uint8_t type;
	uint8_t version;
} __attribute__((packed));
typedef struct efs_key_md efs_key_md_t;

/* @todo remove this key structure when default fs is gone */
struct efs_gi_index_key {
	efs_key_md_t md;
	uint64_t fs_id;
} __attribute((packed));

/* Key for parent -> child mapping.
 * Version 1: key = (parent, enrty_name), value = child.
 * NOTE: This key has variable size.
 * @see efs_parentdir_key for reverse mapping.
 */
struct efs_dentry_key {
	efs_fid_t fid;
	efs_key_md_t md;
	str256_t name;
} __attribute((packed));

/* Key for child -> parent mapping.
 * Version 1: key = (parent, child), value = link_count, where
 * link_count is the amount of links between the parent and the child nodes.
 * @see efs_dentry_key for direct mapping.
 */
struct efs_parentdir_key {
	efs_fid_t fid;
	efs_key_md_t md;
	efs_ino_t pino;
} __attribute__((packed));

/* A generic key type for all attributes (properties) of an inode object */
struct efs_inode_attr_key {
	efs_fid_t fid;
	efs_key_md_t md;
} __attribute__((packed));

/* Max number of hardlinks for an object. */
#define EFS_MAX_LINK UINT32_MAX

/* EFS operations */
/**
 * Gets attributes for a known inode.
 *
 * @note: the call is similar to stat() call in libc. It uses the structure
 * "struct stat" defined in the libC.
 * @param ctx - Filesystem context
 * @param cred - pointer to user's credentials
 * @param ino - pointer to current inode
 * @param stat - [OUT] points to inode's stat
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_getattr(efs_ctx_t ctx, const efs_cred_t *cred,
		const efs_ino_t *ino, struct stat *stat);

/**
 * Sets attributes for a known inode.
 *
 * This call uses a struct stat structure as input. This structure will
 * contain the values to be set. More than one can be set in a single call.
 * The parameter "statflags: indicates which fields are to be considered:
 *  STAT_MODE_SET: sets mode
 *  STAT_UID_SET: sets owner
 *  STAT_GID_SET: set group owner
 *  STAT_SIZE_SET: set size (aka truncate but without unmapping)
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
int efs_setattr(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *ino,
		struct stat *setstat, int statflag);
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
int efs_access(efs_ctx_t ctx, const efs_cred_t *cred,
               const efs_ino_t *ino, int flags);

/** A callback to be used in efs_readddir.
 * @retval true continue iteration.
 * @retval false stop iteration.
 */
typedef bool (*efs_readdir_cb_t)(void *ctx, const char *name,
				 const efs_ino_t *ino);

/** Walk over a directory "dir_ino" and call cb(cb_ctx, entry_name, entry_ino)
 * for each dentry.
 * @param fs_ctx - File system context
 * @param cred -  pointer to user's credentials
 * @param dir_no - pointer to directory inode
 * @param cb - Readdir callback to be called for each dir entry
 * @param cb_ctx - Callback context
 * @retval 0 on success, errno on error.
 */
int efs_readdir(efs_ctx_t fs_ctx, const efs_cred_t *cred,
		const efs_ino_t *dir_ino,
		efs_readdir_cb_t cb,
		void *cb_ctx);
/**
 * Creates a directory.
 *
 * @param ctx - pointer to filesystem context
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be created
 * @param mode - Unix mode for the new entry
 * @paran newdir - [OUT] if successfuly, will point to newly created inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_mkdir(efs_ctx_t *ctx, efs_cred_t *cred, efs_ino_t *parent, char *name,
	      mode_t mode, efs_ino_t *newdir);

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
int efs_lookup(efs_ctx_t *ctx, efs_cred_t *cred, efs_ino_t *parent,
               char *name, efs_ino_t *ino);

/** Hints for "efs_rename" call.
 */
struct efs_rename_flags {
	/** Destination file is open. Rename should not remove such an object
	 * from the filesystems. The object will be unliked from the tree
	 * instead of being destroyed.
	 */
	bool is_dst_open:1;
};
#define EFS_RENAME_FLAGS_INIT  { .is_dst_open = false, }

/**
 * Renames an entry in a filesystem.
 * NOTE: The call has 3 optional paramters (psrc, pdst, flags) which
 * allows it to avoid redundant lookup() calls and could change its behavior.
 *
 * @param[in] fs_ctx - A context associated with the filesystem.
 * @param[in] cred - User's credentials.
 * @param[in] sino - Inode of the source dir.
 * @param[in] sname - Name of the entry within `sino`.
 * @param[in, opt] psrc - Inode of the source object. When NULL is specified,
 *                      the function does a lookup() call internally.
 * @param[in] dino - Inode of the destination dir.
 * @param[in] dname - Name of the entry within `dino`.
 * @param[in, opt] pdst - Inode of the destination object. When NULL is specified,
 *                      the function does a lookup() call internally.
 * @return 0 if successfull, otherwise -errno.
 */
int efs_rename(efs_fs_ctx_t fs_ctx, efs_cred_t *cred,
		efs_ino_t *sino_dir, char *sname, const efs_ino_t *psrc,
		efs_ino_t *dino_dir, char *dname, const efs_ino_t *pdst,
		const struct efs_rename_flags *pflags);


/**
 * Removes a directory. It won't be deleted if not empty.
 * @param ctx - A context associated with a filesystem
 * @param cred - pointer to user's credentials
 * @param parent - pointer to parent directory's inode.
 * @param name - name of the directory to be remove.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_rmdir(efs_ctx_t *ctx, efs_cred_t *cred, efs_ino_t *parent,
              char *name);

/**
 * Removes a file or a symbolic link.
 * Destroys the link between 'dir' and 'fino' and removes
 * the 'fino' object from the namespace if it has no links.
 *
 * @param[in] fs_ctx - A context associated with the filesystem.
 * @param[in] cred - User's credentials.
 * @param[in] dir - Inode of the parent directory.
 * @param[in, opt] fino - Inode of the object to be removed. If NULL is set
 *                        then efs_unlink() will do an extra lookup() call.
 * @param[in] name - Name of the entry to be removed.
 * @return 0 if successfull, otherwise -errno.
 *
 * @see ::efs_destroy_orphaned_file and ::efs_detach.
 */
int efs_unlink(efs_ctx_t *ctx, efs_cred_t *cred, efs_ino_t *dir,
               efs_ino_t *fino, char *name);

/** Removes a link between the parent inode and a filesystem object
 * linked into it with the dentry name.
 */
int efs_detach(efs_fs_ctx_t fs_ctx, const efs_cred_t *cred,
               const efs_ino_t *parent, const efs_ino_t *obj,
               const char *name);

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
int efs_symlink(efs_fs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *parent,
		char *name, char *content, efs_ino_t *newlnk);

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
int efs_link(efs_ctx_t fs_ctx, efs_cred_t *cred, efs_ino_t *ino,
	     efs_ino_t *dino, char *dname);

/** Destroys a file or a symlink object if it has no links in the file system.
 * NOTE: It does nothing if the object has one or more links.
 */
int efs_destroy_orphaned_file(efs_fs_ctx_t fs_ctx, const efs_ino_t *ino);

/* Xattr APIs */
/**
 * Sets an xattr to an inode entry
 * @note: xattr's name are zero-terminated strings.
 *
 * @param[in] fctx - File system ctx
 * @param[in] cred - pointer to user's credentials
 * @param[in] ino - entry's inode
 * @param[in] name - xattr's name
 * @param[in] value - buffer with xattr's value
 * @param[in] size - size of previous buffer
 * @param[in] flags - overwrite behavior, see "man 2 setxattr". Overwrite is
 * prohibited if set to XATTR_CREATE. The valid values of flags are
 * 0, XATTR_CREATE and XATTR_REPLACE
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */

int efs_setxattr(efs_fs_ctx_t fctx, const efs_cred_t *cred,
		 const efs_ino_t *ino, const char *name, char *value,
		 size_t size, int flags);

/**
 * Gets an xattr for an entry
 * @note: xattr's name is a zero-terminated string.
 *
 * @param[in] fctx - File system ctx
 * @param[in] cred - pointer to user's credentials
 * @param[in] ino - entry's inode
 * @param[in] name - xattr's name
 * @param[out] value - buffer in which xattr's value has been copied
 * @param[in,out] size - in: Size of passed value buffer..
 *			 out: A caller can estimate the size of xattr by setting
 *			 this to zero to return the current size of the xattr.
 *
 * @return size of xattr if successful,a negative "-errno" value in case of
 *         failure
 */

ssize_t efs_getxattr(efs_fs_ctx_t fctx, efs_cred_t *cred,
		     const efs_ino_t *ino, const char *name, char *value,
		     size_t *size);

/**
 * Fetches xattr names for a passed inode.
 * @note: xattr's name is a zero-terminated string.
 *
 * @param[in] fctx - File system ctx
 * @param[in] fid - 128 bit File (object) identifier.
 * @param[out] buf - The list which has the set of (null-terminated)
 *		     names, one after the other.
 * @param[out] count - Count of extended attribute names in the buf
 * @param[in] size -Size of buf. If the passed size is zero, the size is set to
 *		    current size of the the fetched attribute name list
 *	 [out] size - Size of the fetched attribute name list.
 *
 * @return zero for success, negative "-errno" value
 *	    in case of failure
 */
int efs_listxattr(efs_ctx_t fs_ctx, const efs_cred_t *cred,
		  const efs_ino_t *ino, void *buf, size_t *count,
		  size_t *size);
/**
 * Removes an xattr
 * @note: xattr's name are zero-terminated strings.
 *
 * @param fs_ctx - filesystem context pointer
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 * @param name - name of xattr to be removed
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_removexattr(efs_ctx_t fs_ctx, const efs_cred_t *cred,
		    const efs_ino_t *ino, const char *name);

/**
 * Removes all xattr for an inode
 *
 * @param fs_ctx - filesystem context pointer
 * @param cred - pointer to user's credentials
 * @param ino - entry's inode
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_remove_all_xattr(efs_ctx_t fs_ctx, efs_cred_t *cred, efs_ino_t *ino);

#endif
