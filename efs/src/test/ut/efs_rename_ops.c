/*
 * Filename: efs_rename_ops.c
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
#include "efs_fs.h"

struct ut_efs_params ut_efs_obj;

/**
 * Test for renaming file
 * Description: rename a file.
 * Strategy:
 *  1. Create a file
 *  2. Rename file.
 *  3. Lookup for new file name
 *  4. Lookup for old file name.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup for new file name should successful.
 *  3. The lookup for old file name should fail with error ENOENT.
 * Environment
 *  1. Delete file.
 */
static void rename_file(void)
{
	int rc = 0;

	char *old_name = "test_rename",
		*new_name = "test_rename_newname";

	efs_ino_t file_inode = 0LL;

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, old_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	 rc = efs_rename(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.current_inode, old_name, NULL,
				&ut_efs_obj.current_inode, new_name, NULL, NULL);

	ut_assert_int_equal(rc, 0);

	file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, new_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	file_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, old_name, &file_inode);

	ut_assert_int_equal(rc, -ENOENT);

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, new_name);
}

/**
 * Test for renaming inito empty directory
 * Description: rename a directory
 * Strategy:
 *  1. Create a directory d1
 *  2. Create a directory d2
 *  3. Rename d1 into d2
 *  3. Lookup for d2
 *  4. Verify the output.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup for d2 should successful.
 * Environment
 *  1. Delete d2.
 */
static void rename_into_empty_dir(void)
{
	int rc = 0;
	char *old_name = "test_rename_dir",
		*new_name = "test_rename_newdir";

	efs_ino_t dir1_inode = 0LL, dir2_inode = 0LL;

	rc = efs_mkdir(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, old_name, 0755, &dir1_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, new_name, 0755, &dir2_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_rename(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, old_name, NULL,
			&ut_efs_obj.current_inode, new_name, NULL, NULL);

	ut_assert_int_equal(rc, 0);

	dir2_inode = 0LL;
	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, new_name, &dir2_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(dir1_inode, dir2_inode);

	dir2_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, old_name, &dir2_inode);

	ut_assert_int_equal(rc, -ENOENT);

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, new_name);
}

/**
 * Test for renaming to nonempty directory
 * Description: rename a directory.
 * Strategy:
 *  1. Create a directory d1
 *  2. Create a directory d2
 *  3. Create directory d3 inside d2
 *  3. Rename d1 into d2
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The Rename operation should fail with EEXIST
 * Environment
 *  1. Delete d3.
 *  2. Delete d2.
 *  3. Delete d1.
 */
static void rename_into_nonempty_dir(void)
{
	int rc = 0;
	char *d1_name = "d1",
		*d2_name = "d2",
		*d3_name = "d3";

	efs_ino_t d1_inode = 0LL, d2_inode = 0LL, d3_inode = 0LL;

	rc = efs_mkdir(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, d1_name, 0755, &d1_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, d2_name, 0755, &d2_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &d2_inode,
			d2_name, 0755, &d3_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_rename(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, d1_name, NULL,
			&ut_efs_obj.current_inode, d2_name, NULL, NULL);

	ut_assert_int_equal(rc, -EEXIST);

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &d2_inode, NULL,
			d3_name);

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, d2_name);

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, d1_name);
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/rename_ops.log";

	printf("Rename tests\n");

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
		ut_test_case(rename_file),
		ut_test_case(rename_into_empty_dir),
		ut_test_case(rename_into_nonempty_dir),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	ut_efs_fs_teardown();

	ut_fini();

	printf("Total tests  = %d\n", test_count);
	printf("Tests passed = %d\n", test_count-test_failed);
	printf("Tests failed = %d\n", test_failed);

	return 0;
}
