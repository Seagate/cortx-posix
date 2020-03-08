/*
 * Filename: fs.h
 * Description: EFS Filesystem functions API.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Satendra Singh <satendra.singh@seagate.com>
*/

#ifndef _FS_H_
#define _FS_H_

#include <sys/queue.h> /* list */
#include <str.h> /* str256_t */
#include "efs.h" /* efs_tree_create_root */
#include "namespace.h" /* namespace methods */

//Forward declaration.
struct efs_fs;
struct efs_fs_node;
/******************************************************************************/

/** Initialize efs_fs: populate in memory list for available fs in kvs
 *
 *  @param cfg[in] - collection item.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_fs_init(struct collection_item *cfg);

/** finalize efs_fs: free up the in memory list.
 *
 * @param void.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_fs_fini(void);

/**
 * Create FileSystem..
 *
 * @fs_name - file system name structure.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_fs_create(const str256_t *fs_name);

/**
 * Detele FileSystem..
 * This will delete the in memmory object of link list.
 * @fs_name - file system name structure.
 * 
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_fs_delete(const str256_t *fs_name);

/**
 *  lookup fs node in fs list.
 *
 *  @param[in]: fs name.
 *  @param[out]: fs node object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_fs_lookup(const str256_t *name, struct efs_fs **fs);

/** scans the namespace table.
 * Callback needs to copy the buffer containing ns, as it will be deleted in
 * the next iteration.
 * @param : function pointer.
 * @param : args call-back context 
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
void efs_fs_scan(void (*fs_scan_cb)(const struct efs_fs *fs, void *args),
				 void *args);

/** gets fs name.
 *
 * @param[in]: namespace object.
 * @param[out]: fs name.
 *
 * return void.
 */
void efs_fs_get_name(const struct efs_fs *fs, str256_t **name);

/**
 * High level API: Open a fs context for the passed fs id.
 * @param fs_name[in]: Filesystem name
 * @param index[out]: Initialized index for that fs_name.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_fs_open(const char *fs_name, struct kvs_idx *index);

/**
 * High level API: Close a fs context.
 * @param fs_ctx: fs_ctx to be finalized
 */
void efs_fs_close(efs_fs_ctx_t fs_ctx);

/* @todo after fs mgmt is tested remove efs_fs_set_fid and efs_fs_get_fid*/

/**
 * @param[in] fs_id - FSID of a filesystem.
 * @return 0 (succ), -ENOMEM (failed to allocate memory), -errno other errors.
 */
int efs_fs_set_fid(efs_fsid_t fs_id);

/**
 * Find FID of a efs  filesystem using its FSID as a key.
 * The functions looks up the global table to get
 * the FID of a filesystem associated with the given FSID.
 * If there is no a filesystem associated with this FSID
 * then the function returns -ENOENT
 *
 * @param[in] fs_id - FSID of a filesystem.
 * @param[out] fid - Found FID of this filesystem.
 * @return 0 (succ), -ENOENT (no FS found), -errno other errors.
 */
int efs_fs_get_fid(efs_fsid_t fs_id, kvs_idx_fid_t *fid);
#endif /* _FS_H_ */
