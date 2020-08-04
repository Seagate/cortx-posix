/*
 * Filename: fs.h
 * Description: EFS filesystemfunctions API.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Satendra Singh <satendra.singh@seagate.com>
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 */

#ifndef _FS_H_
#define _FS_H_

#include <sys/queue.h> /* list */
#include <str.h> /* str256_t */
#include "efs.h" /* efs_tree_create_root */
#include "namespace.h" /* namespace methods */

/* Shared with uper layer, having fs and export informations */
struct efs_fs_list_entry {
	str256_t *fs_name;
	const char *endpoint_info;
};

/******************************************************************************/

/** Initialize efs_fs: populate in memory list for available fs in kvs
 *
 *  @param e_ops: const pointer to endpoint operations.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_fs_init(const struct efs_endpoint_ops *e_ops);

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
 * @param : fs_scan_cb fFunction to be executed for each found filesystem.
 * @param : args call-back context
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_fs_scan_list(int (*fs_scan_cb)(const struct efs_fs_list_entry *list,
		     void *args), void *args);

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
 * @param fs_name[in]: filesystemname
 * @param index[out]: Initialized index for that fs_name.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_fs_open(const char *fs_name, struct efs_fs **fs);

/**
 * High level API: Close a fs context.
 * @param fs_ctx: fs_ctx to be finalized
 */
void efs_fs_close(struct efs_fs *efs_fs);

/* getter: get the filesystem id for given fs object.
 *
 * @param fs[in]: fs object.
 * @param ns_id[out]: filesystem id.
 *
 * return void.
 */
void efs_fs_get_id(struct efs_fs *fs, uint16_t *fs_id);

/*****************************************************************************/
/* efs fs_endpoint */

/** Initialize efs_fs: populate in memory list for available endpoint in kvs
 *
 *  @param void.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_endpoint_init(void);

/** finalize efs_fs: set tenant to null in memory list.
 *
 * @param void.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int efs_endpoint_fini(void);

/**
 * Create FileSystem. endpoint
 *
 * @param endpoint_name[in] - file system endpoint name.
 * @param endpoint_options[in] - file system endpoint name.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_endpoint_create(const str256_t *endpoint_name, const char *endpoint_options);

/**
 * Detele FileSystem endpoint.
 * This will delete the in memmory object of link list.
 *
 * @param endpoint_name[in] - file system name structure.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_endpoint_delete(const str256_t *name);

/**
 * getter function for endpoint information for given efs_fs object.
 *
 * @param fs[in]: efs_fs object.
 * @param info[out]: information associated with an endpoint object
 *
 * @ return void.
 */
void efs_fs_get_endpoint(const struct efs_fs *fs, void **info);

/**
 * Initialize the sys attribute for root's id which will be used for
 * inode number generation (ref. EFS_SYS_ATTR_INO_NUM_GEN)
 *
 * @param efs_fs - Valid file system context.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_ino_num_gen_init(struct efs_fs *efs_fs);

/**
 * Deinit the sys attribute for root's id which was setup earlier via
 * efs_setup_ino_num_gen()
 *
 * @param efs_fs - Valid file system context.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_ino_num_gen_fini(struct efs_fs *efs_fs);

#endif /* _FS_H_ */
