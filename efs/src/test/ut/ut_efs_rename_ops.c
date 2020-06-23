/*
 * Filename: ut_efs_rename_ops.c
 * Description: Implementation tests for rename
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
#include "ut_efs_helper.h"
#define RENAME_ENV_FROM_STATE(__state) (*((struct ut_rename_env **)__state))

struct ut_rename_env {
	struct ut_efs_params ut_efs_obj;
	char **name_list;
};

/**
 * Setup for renaming file test
 * Description: Create a file.
 * Strategy:
 *  1. Create a file
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. File creation should be successful.
 */
static int rename_file_setup(void **state)
{
	int rc = 0;
	char *old_name = "test_rename";

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);

	ut_rename_obj->name_list[0] = old_name;
	ut_rename_obj->ut_efs_obj.file_name = old_name;

	ut_file_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for renaming file
 * Description: rename a file.
 * Strategy:
 *  1. Rename file.
 *  2. Lookup for new file name
 *  3. Lookup for old file name.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup for new file name should successful.
 *  3. The lookup for old file name should fail with error ENOENT.
 */
static void rename_file(void **state)
{
	int rc = 0;
	efs_ino_t file_inode = 0LL;

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_rename_obj->ut_efs_obj;

	char *new_name = "test_rename_newname",
		*old_name = ut_rename_obj->name_list[0];

	rc = efs_rename(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
				&ut_efs_obj->current_inode, old_name, NULL,
				&ut_efs_obj->current_inode, new_name, NULL,
				NULL);

	ut_assert_int_equal(rc, 0);

	ut_rename_obj->name_list[0] = new_name;
	file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, new_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, old_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Teardown for renaming file test
 * Description: Delete a file.
 * Strategy:
 *  1. Delete a file
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. File deletion should be successful.
 */
static int rename_file_teardown(void **state)
{
	int rc = 0;

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);

	ut_rename_obj->ut_efs_obj.file_name = ut_rename_obj->name_list[0];

	ut_file_delete(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for renaming inito empty directory test
 * Description: Create directories
 * Strategy:
 *  1. Create a directory d1
 *  2. Create a directory d2
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Directory creation operations should be successful
 */
static int rename_into_empty_dir_setup(void **state)
{
	int rc = 0;
	char *old_name = "test_rename_dir",
		*new_name = "test_rename_newdir";

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	ut_rename_obj->name_list[0] = old_name;
	ut_rename_obj->name_list[1] = new_name;

	ut_rename_obj->ut_efs_obj.file_name = old_name;

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);
	
	ut_rename_obj->ut_efs_obj.file_name = new_name;

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for renaming inito empty directory
 * Description: rename a directory
 * Strategy:
 *  1. Rename d1 into d2
 *  2. Lookup for d2
 *  3. Verify the output.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup for d2 should be successful.
 *  3. The lookup for d1 should fail 
 */
static void rename_into_empty_dir(void **state)
{
	int rc = 0;

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_rename_obj->ut_efs_obj;

	char *old_name = ut_rename_obj->name_list[0],
		*new_name = ut_rename_obj->name_list[1];

	efs_ino_t dir1_inode = 0LL, dir2_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, old_name, &dir1_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_rename(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, old_name, NULL,
			&ut_efs_obj->current_inode, new_name, NULL, NULL);

	ut_assert_int_equal(rc, 0);

	dir2_inode = 0LL;
	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, new_name, &dir2_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(dir1_inode, dir2_inode);

	dir2_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, old_name, &dir2_inode);

	ut_assert_int_equal(rc, -ENOENT);

	ut_rename_obj->name_list[0] = NULL;
}

/**
 * Teardown for renaming inito empty directory test
 * Description: Delete directories
 * Strategy:
 *  2. Delete directory d2
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Directory deletion operations should be successful
 */
static int rename_into_empty_dir_teardown(void **state)
{
	int rc = 0, i;

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	
	for(i = 1; i >= 0; i--) {
		if(ut_rename_obj->name_list[i] != NULL) {
			ut_rename_obj->ut_efs_obj.file_name =
				ut_rename_obj->name_list[i];

			ut_dir_delete(state);
			ut_assert_int_equal(rc, 0);
		}
	}

	return rc;
}

/**
 * Setup for renaming to nonempty directory test
 * Description: Create directories.
 * Strategy:
 *  1. Create a directory d1
 *  2. Create a directory d2
 *  3. Create directory d3 inside d2
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Directory creation operations should be successful
 */
static int rename_into_nonempty_dir_setup(void **state)
{
	int rc = 0;

	char *d1_name = "d1",
		*d2_name = "d2",
		*d3_name = "d3";

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_rename_obj->ut_efs_obj;

	ut_rename_obj->name_list[0] = d1_name;
	ut_rename_obj->name_list[1] = d2_name;
	ut_rename_obj->name_list[2] = d3_name;

	ut_rename_obj->ut_efs_obj.file_name = d1_name;
	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	ut_rename_obj->ut_efs_obj.file_name = d2_name;
	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	ut_efs_obj->parent_inode = ut_efs_obj->file_inode;
	ut_efs_obj->file_name = d3_name;

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for renaming to nonempty directory
 * Description: rename a directory.
 * Strategy:
 *  1. Rename d1 into d2
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The Rename operation should fail with EEXIST
 */
static void rename_into_nonempty_dir(void **state)
{
	int rc = 0;

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_rename_obj->ut_efs_obj;

	rc = efs_rename(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_rename_obj->name_list[0],
			NULL, &ut_efs_obj->current_inode,
			ut_rename_obj->name_list[1], NULL, NULL);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Teardown for renaming to nonempty directory test
 * Description: Delete directories.
 * Strategy:
 *  1. Delete d3.
 *  2. Delete d2.
 *  3. Delete d1.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Directory deletion operations should be successful
 */
static int rename_into_nonempty_dir_teardown(void **state)
{
	int rc = 0;

	struct ut_rename_env *ut_rename_obj = RENAME_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_rename_obj->ut_efs_obj;

	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc, 0);

	ut_efs_obj->parent_inode = ut_efs_obj->current_inode;

	ut_efs_obj->file_name = ut_rename_obj->name_list[1];
	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc, 0);

	ut_efs_obj->file_name = ut_rename_obj->name_list[0];
	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for rename_ops test group
 */
static int rename_ops_setup(void **state)
{
	int rc = 0;
	struct ut_rename_env *ut_rename_obj = NULL;

	ut_rename_obj = calloc(sizeof(struct ut_rename_env), 1);

	ut_assert_not_null(ut_rename_obj);

	ut_rename_obj->name_list = calloc(sizeof(char *), 3);
	
	if(ut_rename_obj->name_list == NULL) {
		ut_assert_true(0);
		rc = -ENOMEM;
	}		

	*state = ut_rename_obj;
	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for rename_ops test group
 */
static int rename_ops_teardown(void **state)
{
	int rc = 0;

	rc = ut_efs_fs_teardown(state);
	ut_assert_int_equal(rc, 0);

	free(*state);

	return rc;
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/ut_efs.log";

	printf("Rename tests\n");

	rc = ut_load_config(CONF_FILE);
	if (rc != 0) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_log = ut_get_config("efs", "log_path", test_log);

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		goto out;
	}

	struct test_case test_list[] = {
		ut_test_case(rename_file, rename_file_setup,
				rename_file_teardown),
		ut_test_case(rename_into_empty_dir, rename_into_empty_dir_setup,
				rename_into_empty_dir_teardown),
		ut_test_case(rename_into_nonempty_dir,
				rename_into_nonempty_dir_setup,
				rename_into_nonempty_dir_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, rename_ops_setup,
				rename_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
