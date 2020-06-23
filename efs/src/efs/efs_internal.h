/*
 * Filename:         efs_internal.h
 * Description:      EOS file system internal APIs

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system internal APIs
*/

#ifndef _EFS_INTERNAL_H
#define _EFS_INTERNAL_H

#include <dstore.h>
#include <nsal.h>
#include "efs.h"
#include "kvnode.h"

/** Set the extstore object identifier (kfid) with kvsns inode as the key */
int efs_set_ino_oid(struct efs_fs *efs_fs, efs_ino_t *ino, dstore_oid_t *oid);

/** Get the extstore object identifier for the passed efs inode */
int efs_ino_to_oid(struct efs_fs *efs_fs, const efs_ino_t *ino, dstore_oid_t *oid);

/** Delete the ino-kfid key-val pair from the kvs. Called during unlink/rm. */
int efs_del_oid(struct efs_fs *efs_fs, const efs_ino_t *ino);

/* Initialize the kvnode with given parameters
 *
 * @param[in] node *    - Kvnode pointer which will be initialized using kvnode
 *                        API to hold node information
 * @param[in] tree *    - Kvtree pointer which will be stored in kvnode on init
 * @param[in] ino *     - Inode of a file - file identifier
 * @param[in] bufstat * - Stat attribute buffer, stored in kvnode
 *
 * @return - 0 on sucess on failure error code given by kvnode APIs
 */
int efs_kvnode_init(struct kvnode *node, struct kvtree *tree,
		    const efs_ino_t *ino, const struct stat *bufstat);

/* Load kvnode for given file identifier from disk
 *
 * @param[in] node *    - Kvnode pointer which will be initialized using kvnode
 *                        API to hold node information
 * @param[in] tree *    - Kvtree pointer which will be stored in kvnode on init
 * @param[in] ino *     - Inode of a file - file identifier
 *
 * @return - 0 on sucess on failure error code given by kvnode APIs
 */
int efs_kvnode_load(struct kvnode *node, struct kvtree *tree,
		    const efs_ino_t *ino);

/* Store the stat associated with particular file inode held by given kvnode
 *
 * @param[in] node *    - Kvnode pointer which is already initialized and hold
 *                        the stat attributes which needs to be stored
 *
 * @return - 0 on sucess on failure error code given by kvnode APIs
 */
int efs_set_stat(struct kvnode *node);

/* Extract the stat attribute for particular file held by given kvnode
 *
 * @param[in] node *     - Kvnode pointer which is already initialzed and having
 *                         stat attributes
 * @param[in] bufstat ** - Stat attribute buffer pointer to store stat attribute
 *
 * @return - 0 on sucess on failure error code given by kvnode APIs
 */
int efs_get_stat(struct kvnode *node, struct stat **bufstat);

/* Delete the stat associated with particular file inode held by given kvnode
 *
 * @param[in] node *    - Kvnode pointer which is initialized and needs to be
 *                        deleted
 *
 * @return - 0 on sucess on failure error code given by kvnode APIs
 */
int efs_del_stat(struct kvnode *node);

/* Update the stat hold by given kvnode for given flag and re-store it back to
 * the disk
 *
 * @param[in] node *   - Kvnode pointer which is already initialzed and having
 *                       stat attributes
 * @param[in] flags    - Flag mask for which stats needs to be updated
 *
 * @return - 0 on sucess on failure error code given by kvnode APIs
 */
int efs_update_stat(struct kvnode *node, int flags);
#endif
