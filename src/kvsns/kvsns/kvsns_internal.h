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

/* kvsns_internal.h.c
 * KVSNS: headers for internal functions and macros
 */

#ifndef KVSNS_INTERNAL_H
#define KVSNS_INTERNAL_H

/******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ini_config.h>
#include <kvsns/kvsns.h>
#include <string.h>

/******************************************************************************/
/* Public data types */

/** A string object which has a C string (up to 255 characters)
 * and its length.
 */
struct kvsns_str256 {
	/** The length of the C string. */
	uint8_t s_len;
	/** A buffer which contains a null-terminated C string. */
	char    s_str[NAME_MAX + 1];
} __attribute__((packed));

/** Name of an object in a file tree. */
typedef struct kvsns_str256 kvsns_name_t;

/** Convert a C-string into a FS tree name.
 * @param[in] name - a C-string to be converted.
 * @param[out] kname - a kvsns string.
 * @return Len of the string or -EINVAL.
 */
int kvsns_name_from_cstr(const char *name, kvsns_name_t *kname);

/******************************************************************************/
int kvsns_next_inode(kvsns_ino_t *ino);
int kvsns2_next_inode(void *ctx, kvsns_ino_t *ino);
int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str);
int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str);
int kvsns_create_entry(kvsns_cred_t *cred, kvsns_ino_t *parent,
		       char *name, char *lnk, mode_t mode,
		       kvsns_ino_t *newdir, enum kvsns_type type);
int kvsns2_create_entry(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
			char *name, char *lnk, mode_t mode, kvsns_ino_t *newdir,
			enum kvsns_type type);

/******************************************************************************/
/* Inode Attributes API */
int kvsns2_get_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino,
		       struct stat **bufstat);
int kvsns2_set_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino,
		       struct stat *bufstat);
int kvsns2_del_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino);
int kvsns2_update_stat(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ini, int flags);
int kvsns_get_link(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino,
		   void **buf, size_t *buf_size);
int kvsns_set_link(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino,
		   void *buf, size_t buf_size);
int kvsns_del_link(kvsns_fs_ctx_t ctx, const kvsns_ino_t *ino);

/* deprecated API */
int kvsns_get_stat(kvsns_ino_t *ino, struct stat *bufstat);
int kvsns_set_stat(kvsns_ino_t *ino, struct stat *bufstat);
int kvsns_update_stat(kvsns_ino_t *ino, int flags);
int kvsns_amend_stat(struct stat *stat, int flags);
int kvsns_delall_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino);


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
int kvsns_tree_detach(kvsns_fs_ctx_t fs_ctx,
		      const kvsns_ino_t *parent_ino,
		      const kvsns_ino_t *ino,
		      const kvsns_name_t *node_name);

/******************************************************************************/
/** Create a new link between two nodes in a filesysytem.
 * @param[in] ctx - Filesystem context
 * @param[in] parent_ino - Inode of the parent
 * @param[in] ino - Inode of the child
 * @param[in] node_name - Name of a new entry in the `parent_ino` which will be
 * associated with the given `ino`.
 * @return 0 if successful, otherwise -errno.
 */
int kvsns_tree_attach(kvsns_fs_ctx_t ctx,
		      const kvsns_ino_t *parent_ino,
		      const kvsns_ino_t *ino,
		      const kvsns_name_t *node_name);


/******************************************************************************/
/** Change the name of a link between a parent node and a child node without
 * modifying the link itself.
 */
int kvsns_tree_rename_link(kvsns_fs_ctx_t fs_ctx,
			   const kvsns_ino_t *parent_ino,
			   const kvsns_ino_t *ino,
			   const kvsns_name_t *old_name,
			   const kvsns_name_t *new_name);

/******************************************************************************/
/** Check if the given inode has at least one child inode linked into it. */
int kvsns_tree_has_children(kvsns_fs_ctx_t fs_ctx,
			    const kvsns_ino_t *ino,
			    bool *has_children);

/******************************************************************************/
/* Get the inode of an object linked into a parent inode.
 * @param[in] parent_ino - Inode of the parent object.
 * @param[in] name - Name of the object to be found.
 * @param[out, optional] ino - Inode of the found object.
 * @return 0 if successfull, otherwise -errno, including -ENOENT
 * if the object does not exist.
 */
int kvsns_tree_lookup(kvsns_fs_ctx_t fs_ctx,
		      const kvsns_ino_t *parent_ino,
		      const kvsns_name_t *name,
		      kvsns_ino_t *ino);

/******************************************************************************/
/** Walk over children (dentries) if an inode (directory). */
int kvsns_tree_iter_children(kvsns_fs_ctx_t fs_ctx,
			     const kvsns_ino_t *ino,
			     kvsns_readdir_cb_t cb,
			     void *cb_ctx);

/******************************************************************************/
/** Set the extstore object identifier (kfid) with kvsns inode as the key */
int kvsns_set_ino_kfid(void *ctx, kvsns_ino_t *ino, kvsns_fid_t *kfid);

/** Get the extstore object identifier for the passed kvsns inode */
int kvsns_ino_to_kfid(void *ctx, kvsns_ino_t *ino, kvsns_fid_t *kfid);

/** Delete the ino-kfid key-val pair from the kvs. Called during unlink/rm. */
int kvsns_del_kfid(void *ctx, kvsns_ino_t *ino);

#endif
