/*
 * Filename: link_ops.c
 * Description: Implementation tests for symlinks
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
 * Test for symlink creation
 * Description: create a symlink with ordinary contents.
 * Behavior:
 *  1. Create a symlink with contents.
 *  2. Read the symlink contents.
 *  3. Verify the output against readlink.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The symlink contents matches the input.
 * Environment:
 *  1. Destroy symlink during teardown.
 */
static void create_symlink(void)
{
	int rc = 0;
	char *link_name = "test_symlink",
		*symlink_content = "abcdefghijklmnopqrstuvwxyz";

	efs_ino_t link_inode = 0LL;

	rc = efs_symlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
				&ut_efs_obj.current_inode, link_name,
				symlink_content, &link_inode);

	ut_assert_int_equal(rc, 0);

	char buff[100];
	size_t content_size = 100;

	rc = efs_readlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &link_inode,
				buff, &content_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(content_size, strlen(symlink_content));

	rc = memcmp(buff, symlink_content, strlen(symlink_content));

	ut_assert_int_equal(rc, 0);

	efs_unlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, link_name);
}

/**
 * Test for longname symlink creation
 * Description: create a symlink with long name with ordinary contents.
 * Behavior:
 *  1. Create a long name symlink with contents.
 *  2. Read the symlink contents.
 *  3. Verify the output against readlink.
 * Expected behavior:
 *  1. No errors from EFS API
 *  2. The symlink contents matches the input.
 * Environment
 *  1. Destroy symlink during teardown.
 */
static void create_longname255_symlink(void)
{
	int rc = 0;
	char *long_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901",
		*symlink_content = "abcdefghijklmnopqrstuvwxyz";

	efs_ino_t link_inode = 0LL;

	ut_assert_int_equal(255, strlen(long_name));

	rc = efs_symlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
				&ut_efs_obj.current_inode, long_name,
				symlink_content, &link_inode);

	ut_assert_int_equal(rc, 0);

	char buff[100];
	size_t content_size = 100;

	rc = efs_readlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &link_inode,
				buff, &content_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(content_size, strlen(symlink_content));

	rc = memcmp(buff, symlink_content, strlen(symlink_content));

	ut_assert_int_equal(rc, 0);

	efs_unlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, long_name);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/link_ops.log";

	if (argc > 1) {
		test_log = argv[1];
	}

	printf("Symlink Tests\n");

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
		ut_test_case(create_symlink),
		ut_test_case(create_longname255_symlink),
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
