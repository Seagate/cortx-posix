/*
 * Filename: ut_cortxfs_helper.c
 * Description: Functions for fs_open and fs_close required for test
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

#include "ut_cortxfs_helper.h"
#include "ut_cortxfs_endpoint_dummy.h"

int ut_cfs_fs_setup(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);
	char *ut_conf_file = "/tmp/cortxfs/build-cortxfs/test/ut/ut_cortxfs.conf",
		*def_fs = "testfs";
	ut_cfs_obj->cfs_fs = CFS_NULL_FS_CTX;

	ut_cfs_obj->cred.uid = getuid();
	ut_cfs_obj->cred.gid = getgid();

	ut_cfs_obj->current_inode = CFS_ROOT_INODE;
	ut_cfs_obj->parent_inode = CFS_ROOT_INODE;

	ut_cfs_obj->fs_name = NULL;

	rc = cfs_init(CFS_DEFAULT_CONFIG, get_endpoint_dummy_ops());
	if (rc != 0) {
		fprintf(stderr, "cfs_init: err = %d\n", rc);
		goto out;
	}

	rc = ut_load_config(ut_conf_file);
	if (rc != 0) {
		fprintf(stderr, "ut_load_config: err = %d\n", rc);
		goto out;
	}

	ut_cfs_obj->fs_name = ut_get_config("cortxfs", "fs", def_fs);

	str256_t fs_name;
	str256_from_cstr(fs_name, ut_cfs_obj->fs_name,
				strlen(ut_cfs_obj->fs_name));
	rc = 0;
	rc = cfs_fs_create(&fs_name);

	if (rc != 0) {
		fprintf(stderr, "Failed to create FS %s, rc=%d\n",
			ut_cfs_obj->fs_name, rc);
		goto out;
	}

	struct cfs_fs *cfs_fs = NULL;
	rc = cfs_fs_open(ut_cfs_obj->fs_name, &cfs_fs);

	if (rc != 0) {
		fprintf(stderr, "Unable to open FS for fs_name:%s, rc=%d !\n",
			ut_cfs_obj->fs_name, rc);
		goto out;
	}
	ut_cfs_obj->cfs_fs = cfs_fs;

out:
	return rc;
}

int ut_cfs_fs_teardown(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	cfs_fs_close(ut_cfs_obj->cfs_fs);

	str256_t fs_name;
	str256_from_cstr(fs_name, ut_cfs_obj->fs_name,
				strlen(ut_cfs_obj->fs_name));

	rc = cfs_fs_delete(&fs_name);
	ut_assert_int_equal(rc, 0);

	free(ut_cfs_obj->fs_name);

	rc = cfs_fini();
	ut_assert_int_equal(rc, 0);

	return rc;
}

int ut_file_create(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_creat(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name, 0755,
			&ut_cfs_obj->file_inode);
	if (rc) {
		printf("Failed to create file %s\n", ut_cfs_obj->file_name);
	}

	return rc;
}

int ut_file_delete(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_unlink(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, NULL, ut_cfs_obj->file_name);
	if (rc) {
		printf("Failed to delete file %s\n", ut_cfs_obj->file_name);
	}

	return rc;
}

int ut_dir_create(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name, 0755,
			&ut_cfs_obj->file_inode);
	if (rc) {
		printf("Failed to create directory %s\n", ut_cfs_obj->file_name);
	}

	return rc;
}

int ut_dir_delete(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_rmdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name);
	if (rc) {
		printf("Failed to delete directory %s\n", ut_cfs_obj->file_name);
	}

	return rc;
}
