/*
 * Filename: ut_efs_file_ops.c
 * Description: Implementation tests for file creation
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

/**
 * Setup for file creation test.
 * Description: check if file exists.
 * Strategy:
 *  1. Lookup for file.
 *  2. fail if lookup is successful
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should fail with error -ENOENT.
 */
static int create_file_setup(void **state)
{
	int rc = 0;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	char *file_name = "test_file";
	ut_efs_obj->file_name = file_name;

	efs_ino_t file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, file_name,
			&file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = 0;

	return rc;
}

/**
 * Test for file creation.
 * Description: create a file.
 * Strategy:
 *  1. Create a file.
 *  2. Lookup for created file.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should verify file creation.
 */
static void create_file(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	efs_ino_t file_inode = 0LL;

	rc = efs_creat(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name, 0755,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
			&file_inode);

	ut_assert_int_equal(rc, 0);
}

/**
 * Setup for longname file creation test.
 * Description: check if file exists.
 * Strategy:
 *  1. Lookup for file.
 *  2. Fail if lookup is successful.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should fail with error -ENOENT.
 */
static int create_longname255_file_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	char *file_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901";

	ut_efs_obj->file_name = file_name;
	efs_ino_t file_inode = 0LL;

	ut_assert_int_equal(255, strlen(file_name));

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = 0;

	return rc;
}

/**
 * Test for longname file creation.
 * Description: create a longname file.
 * Strategy:
 *  1. Create a longname file.
 *  2. Lookup for created file.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should verify file creation.
 */
static void create_longname255_file(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	efs_ino_t file_inode = 0LL;

	rc = efs_creat(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name, 0755,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
			&file_inode);

	ut_assert_int_equal(rc, 0);
}

/**
 * Setup for existing file creation test.
 * Description: create file.
 * Strategy:
 *  1. Lookup for file
 *  2. Fail if lookup is successful
 *  3. Create a file.
 *  4. Lookup for created file.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. First lookup should fail with error -ENOENT
 *  3. Second lookup should be successful 
 */
static int create_exist_file_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	char * file_name = "test_existing_file";
	efs_ino_t file_inode = 0LL;

	ut_efs_obj->file_name = file_name;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = 0;

	rc = efs_creat(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for existing file creation.
 * Description: create a existing file.
 * Strategy:
 *  1. Create file.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. File creation should fail with error -EEXIST.
 */
static void create_exist_file(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);	

	efs_ino_t file_inode = 0LL;

	rc = efs_creat(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name, 0755,
			&file_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * teardown for file test.
 * Description: delete file.
 * Strategy:
 *  1. Delete file.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Delete should be successful
 */
static int file_test_teardown(void **state)
{
	int rc = 0;

	rc = ut_file_delete(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for file_ops test group.
 */
static int file_ops_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = NULL;

	ut_efs_obj = calloc(sizeof(struct ut_efs_params), 1);

	ut_assert_not_null(ut_efs_obj);

	*state = ut_efs_obj;
	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/*
 * Teardown for file_ops test group.
 */
static int file_ops_teardown(void **state)
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
	char *test_log = "/var/log/eos/test/ut/ut_efs.log";

	printf("File creation tests\n");

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
		ut_test_case(create_file, create_file_setup, file_test_teardown),
		ut_test_case(create_longname255_file,
				create_longname255_file_setup, file_test_teardown),
		ut_test_case(create_exist_file, create_exist_file_setup,
				file_test_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, file_ops_setup, file_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
