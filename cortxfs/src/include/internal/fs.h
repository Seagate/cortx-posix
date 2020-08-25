/*
 * Filename: fs.h
 * Description: CORTXFS filesystemfunctions API.
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
 */

#ifndef _FS_H_
#define _FS_H_

#include <sys/queue.h> /* list */
#include <str.h> /* str256_t */
#include "cortxfs.h" /* cfs_tree_create_root */
#include "namespace.h" /* namespace methods */

/* Shared with uper layer, having fs and export informations */
struct cfs_fs_list_entry {
	str256_t *fs_name;
	const char *endpoint_info;
};

/******************************************************************************/

/** Initialize cfs_fs: populate in memory list for available fs in kvs
 *
 *  @param e_ops: const pointer to endpoint operations.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int cfs_fs_init(const struct cfs_endpoint_ops *e_ops);

/** finalize cfs_fs: free up the in memory list.
 *
 * @param void.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int cfs_fs_fini(void);

/**
 * Create FileSystem..
 *
 * @fs_name - file system name structure.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_fs_create(const str256_t *fs_name);

/**
 * Detele FileSystem..
 * This will delete the in memmory object of link list.
 * @fs_name - file system name structure.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_fs_delete(const str256_t *fs_name);

/**
 *  lookup fs node in fs list.
 *
 *  @param[in]: fs name.
 *  @param[out]: fs node object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int cfs_fs_lookup(const str256_t *name, struct cfs_fs **fs);

/** scans the namespace table.
 * Callback needs to copy the buffer containing ns, as it will be deleted in
 * the next iteration.
 * @param : fs_scan_cb fFunction to be executed for each found filesystem.
 * @param : args call-back context
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int cfs_fs_scan_list(int (*fs_scan_cb)(const struct cfs_fs_list_entry *list,
		     void *args), void *args);

/** gets fs name.
 *
 * @param[in]: namespace object.
 * @param[out]: fs name.
 *
 * return void.
 */
void cfs_fs_get_name(const struct cfs_fs *fs, str256_t **name);

/**
 * High level API: Open a fs context for the passed fs id.
 * @param fs_name[in]: filesystemname
 * @param index[out]: Initialized index for that fs_name.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_fs_open(const char *fs_name, struct cfs_fs **fs);

/**
 * High level API: Close a fs context.
 * @param fs_ctx: fs_ctx to be finalized
 */
void cfs_fs_close(struct cfs_fs *cfs_fs);

/* getter: get the filesystem id for given fs object.
 *
 * @param fs[in]: fs object.
 * @param ns_id[out]: filesystem id.
 *
 * return void.
 */
void cfs_fs_get_id(struct cfs_fs *fs, uint16_t *fs_id);

/*****************************************************************************/
/* cortxfs fs_endpoint */

/** Initialize cfs_fs: populate in memory list for available endpoint in kvs
 *
 *  @param void.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int cfs_endpoint_init(void);

/** finalize cfs_fs: set tenant to null in memory list.
 *
 * @param void.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int cfs_endpoint_fini(void);

/**
 * Create FileSystem. endpoint
 *
 * @param endpoint_name[in] - file system endpoint name.
 * @param endpoint_options[in] - file system endpoint name.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_endpoint_create(const str256_t *endpoint_name, const char *endpoint_options);

/**
 * Detele FileSystem endpoint.
 * This will delete the in memmory object of link list.
 *
 * @param endpoint_name[in] - file system name structure.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_endpoint_delete(const str256_t *name);

/**
 * getter function for endpoint information for given cfs_fs object.
 *
 * @param fs[in]: cfs_fs object.
 * @param info[out]: information associated with an endpoint object
 *
 * @ return void.
 */
void cfs_fs_get_endpoint(const struct cfs_fs *fs, void **info);

/**
 * Initialize the sys attribute for root's id which will be used for
 * inode number generation (ref. CFS_SYS_ATTR_INO_NUM_GEN)
 *
 * @param cfs_fs - Valid file system context.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_ino_num_gen_init(struct cfs_fs *cfs_fs);

/**
 * Deinit the sys attribute for root's id which was setup earlier via
 * cfs_setup_ino_num_gen()
 *
 * @param cfs_fs - Valid file system context.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int cfs_ino_num_gen_fini(struct cfs_fs *cfs_fs);

#endif /* _FS_H_ */
