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

/* forword declations */
struct kvs_idx;

/**
 * Start the efs library. This should be done by every thread using the library
 *
 * @note: this function will allocate required resources and set useful
 * variables to their initial value. As the programs ends efs_init() should be
 * invoked to perform all needed cleanups.
 * In this version of the API, it takes no parameter, but this may change in
 * later versions.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_init(const char *config);

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
int efs_get_stat(efs_fs_ctx_t ctx, const efs_ino_t *ino,
                 struct stat **bufstat);
int efs_set_stat(efs_fs_ctx_t ctx, const efs_ino_t *ino,
                 struct stat *bufstat);
int efs_del_stat(efs_fs_ctx_t ctx, const efs_ino_t *ino);
int efs_update_stat(efs_fs_ctx_t ctx, const efs_ino_t *ini, int flags);

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

/** A callback to be used in kvsns_readddir.
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

#endif

