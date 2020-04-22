/*
 * Filename: efs_ut_helper.c
 * Description: Functions for fs_open and fs_close required for test
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Shraddha Shinde <shraddha.shinde@seagate.com>
 */

#include "efs_fs.h"

struct collection_item *cfg_items;

static int get_ut_config(char **fs_name)
{

	int rc = 0;
	char *ut_config_file = "/tmp/eos-fs/build-efs/test/ut/efs_ut.ini";
	struct collection_item *errors = NULL;
	struct collection_item *item = NULL;

	rc = config_from_file("libkvsns", ut_config_file, &cfg_items,
				INI_STOP_ON_ERROR, &errors);
	if (rc != 0) {
		return rc;
	}

	rc = get_config_item("config", "fs", cfg_items, &item);
	if (rc != 0) {
		return rc;
	}

	*fs_name = get_string_config_value(item, NULL);

	return rc;
}

int ut_efs_fs_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->efs_fs = EFS_NULL_FS_CTX;

	ut_efs_obj->cred.uid = getuid();
	ut_efs_obj->cred.gid = getgid();

	ut_efs_obj->current_inode = EFS_ROOT_INODE;
	ut_efs_obj->parent_inode = EFS_ROOT_INODE;

	ut_efs_obj->fs_name = NULL;

	rc = efs_init(EFS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "efs_init: err = %d\n", rc);
		goto out;
	}

	rc = get_ut_config(&ut_efs_obj->fs_name);
	if (rc != 0) {
		fprintf(stderr, "Failed to get ut config: err = %d\n", rc);
		goto out;
	}

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
