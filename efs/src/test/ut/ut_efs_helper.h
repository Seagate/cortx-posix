/*
 * Filename: efs_fs.h
 * Description:  Declararions of functions used to test efs_fs
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

#ifndef TEST_EFS_FS_H
#define TEST_EFS_FS_H

#include <string.h>
#include <errno.h>
#include <dstore.h>
#include "str.h"
#include "internal/fs.h"
#include "namespace.h"
#include "ut.h"
#include "efs.h"

#define DEFAULT_CONFIG "/etc/cortx/cortxfs.conf"
#define CONF_FILE "/tmp/efs/build-efs/test/ut/ut_efs.conf"
#define ENV_FROM_STATE(__state) (*((struct ut_efs_params **)__state))

struct ut_efs_params {
	struct efs_fs *efs_fs;
	efs_cred_t cred;
	efs_ino_t file_inode, current_inode, parent_inode;
	struct kvstore *kvstor;
	char *file_name;
	char *fs_name;
};

struct collection_item *cfg_items;

/**
 * Initialization for NS tests
 */
int ut_efs_fs_setup(void **state);

/*
 * Finishes initialization done for NS tests
 */ 
int ut_efs_fs_teardown(void **state);

/*
 * Creates a file
 */
int ut_file_create(void **state);

/*
 * Deletes a file
 */
int ut_file_delete(void **state);

/*
 * Creates a directory
 */
int ut_dir_create(void **state);

/*
 * Deletes a directory
 */
int ut_dir_delete(void ** state);
#endif /* TEST_EFS_FS_H */
