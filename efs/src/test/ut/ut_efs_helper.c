/*
 * Filename: ut_efs_helper.c
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

#include "ut_efs_helper.h"
#include "ut_efs_endpoint_dummy.h"

int ut_efs_fs_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);
	char *ut_conf_file = "/tmp/eos-fs/build-efs/test/ut/ut_efs.conf",
		*def_fs = "testfs";
	ut_efs_obj->efs_fs = EFS_NULL_FS_CTX;

	ut_efs_obj->cred.uid = getuid();
	ut_efs_obj->cred.gid = getgid();

	ut_efs_obj->current_inode = EFS_ROOT_INODE;
	ut_efs_obj->parent_inode = EFS_ROOT_INODE;

	ut_efs_obj->fs_name = NULL;

	rc = efs_init(EFS_DEFAULT_CONFIG, get_endpoint_dummy_ops());
	if (rc != 0) {
		fprintf(stderr, "efs_init: err = %d\n", rc);
		goto out;
	}

	rc = ut_load_config(ut_conf_file);
	if (rc != 0) {
		fprintf(stderr, "ut_load_config: err = %d\n", rc);
		goto out;
	}

	ut_efs_obj->fs_name = ut_get_config("efs", "fs", def_fs);

	str256_t fs_name;
	str256_from_cstr(fs_name, ut_efs_obj->fs_name,
				strlen(ut_efs_obj->fs_name));
	rc = 0;
	rc = efs_fs_create(&fs_name);

	if (rc != 0) {
		fprintf(stderr, "Failed to create FS %s, rc=%d\n",
			ut_efs_obj->fs_name, rc);
		goto out;
	}

	struct efs_fs *efs_fs = NULL;
	rc = efs_fs_open(ut_efs_obj->fs_name, &efs_fs);

	if (rc != 0) {
		fprintf(stderr, "Unable to open FS for fs_name:%s, rc=%d !\n",
			ut_efs_obj->fs_name, rc);
		goto out;
	}
	ut_efs_obj->efs_fs = efs_fs;

out:
	return rc;
}

int ut_efs_fs_teardown(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	efs_fs_close(ut_efs_obj->efs_fs);

	str256_t fs_name;
	str256_from_cstr(fs_name, ut_efs_obj->fs_name,
				strlen(ut_efs_obj->fs_name));

	rc = efs_fs_delete(&fs_name);
	ut_assert_int_equal(rc, 0);

	free(ut_efs_obj->fs_name);

	rc = efs_fini();
	ut_assert_int_equal(rc, 0);

	return rc;
}

int ut_file_create(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_creat(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name, 0755,
			&ut_efs_obj->file_inode);
	if (rc) {
		printf("Failed to create file %s\n", ut_efs_obj->file_name);
	}

	return rc;
}

int ut_file_delete(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_unlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, NULL, ut_efs_obj->file_name);
	if (rc) {
		printf("Failed to delete file %s\n", ut_efs_obj->file_name);
	}

	return rc;
}

int ut_dir_create(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name, 0755,
			&ut_efs_obj->file_inode);
	if (rc) {
		printf("Failed to create directory %s\n", ut_efs_obj->file_name);
	}

	return rc;
}

int ut_dir_delete(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_rmdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name);
	if (rc) {
		printf("Failed to delete directory %s\n", ut_efs_obj->file_name);
	}

	return rc;
}
