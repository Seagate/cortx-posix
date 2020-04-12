/*
 * Filename: efs_file_ops.c
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
#include "efs_fs.h"

struct ut_efs_params ut_efs_obj;

/**
 * Test for file creation.
 * Description: create a file.
 * Strategy:
 *  1. Check if file exists, fail if exists
 *  2. Create a file.
 *  3. Lookup for created file.
 *  4. Verify output.
 * Expected ehavior:
 *  1. No errors from EFS API.
 *  2. The lookup should verify file creation.
 * Environment:
 *  1.Delete created file during teardown.
 */
static void create_file(void)
{
	int rc = 0;
	char *file_name = "test_file";

	efs_ino_t file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, NULL, file_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for longname file creation.
 * Description: create a longname file.
 * Strategy:
 *  1. Check if file exists, fail if exists
 *  2. Create a longname file.
 *  3. Lookup for created file.
 *  4. Verify output.
 * Expected ehavior:
 *  1. No errors from EFS API.
 *  2. The lookup should verify file creation.
 * Environment:
 *  1.Delete created file during teardown.
 */
static void create_longname255_file(void)
{
	int rc = 0;
	char *file_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901";
	efs_ino_t file_inode = 0LL;

	ut_assert_int_equal(255, strlen(file_name));

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, NULL, file_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for existing file creation.
 * Description: create a existing file.
 * Strategy:
 *  1. Check if file exists, fail if exists
 *  2. Create a file.
 *  3. Lookup for created file.
 *  4. Create same file again.
 * Expected ehavior:
 *  1. No errors from EFS API.
 *  2. Second time file creation should fail.
 * Environment:
 *  1.Delete created file during teardown.
 */
static void create_exist_file(void)
{
	int rc = 0;
	char *file_name = "test_existing_file";

	efs_ino_t file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, -EEXIST);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, NULL, file_name);

	ut_assert_int_equal(rc, 0);
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/file_ops.log";

	printf("File creation tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = ut_efs_fs_setup();
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(create_file, NULL, NULL),
		ut_test_case(create_longname255_file, NULL, NULL),
		ut_test_case(create_exist_file, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	ut_efs_fs_teardown();

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
