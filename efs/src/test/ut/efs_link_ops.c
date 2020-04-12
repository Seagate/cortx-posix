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
 * Strategy:
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

	rc = efs_symlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.current_inode, link_name,
				symlink_content, &link_inode);

	ut_assert_int_equal(rc, 0);

	char buff[100];
	size_t content_size = 100;

	rc = efs_readlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &link_inode,
				buff, &content_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(content_size, strlen(symlink_content));

	rc = memcmp(buff, symlink_content, strlen(symlink_content));

	ut_assert_int_equal(rc, 0);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, link_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for longname symlink creation
 * Description: create a symlink with long name with ordinary contents.
 * Strategy:
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

	rc = efs_symlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.current_inode, long_name,
				symlink_content, &link_inode);

	ut_assert_int_equal(rc, 0);

	char buff[100];
	size_t content_size = 100;

	rc = efs_readlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &link_inode,
				buff, &content_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(content_size, strlen(symlink_content));

	rc = memcmp(buff, symlink_content, strlen(symlink_content));

	ut_assert_int_equal(rc, 0);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, long_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for hardlink creation
 * Description: Create a hardlink for file
 * Strategy:
 *  1. Create hardlink for file
 *  2. Lookup for hardlink
 *  3. Verify output
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. Lookup for hardlink should be successful
 * Environment:
 *  1. Delete the created hardlink
 */
static void create_hardlink(void)
{
	int rc = 0;
	char *link_name = "test_hardlink";
	efs_ino_t file_inode = 0LL;

	struct stat stat_out;
	memset(&stat_out, 0, sizeof(stat_out));

	time_t cur_time;
	time(&cur_time);

	rc = efs_link(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.file_inode, &ut_efs_obj.current_inode,
			link_name);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, link_name,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj.file_inode, file_inode);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_nlink, 2);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	} 

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, link_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for longname hardlink creation
 * Description: Create a hardlink with longname for file
 * Strategy:
 *  1. Create longname hardlink for file
 *  2. Lookup for hardlink
 *  3. Verify output
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. Lookup for hardlink should be successful
 * Environment:
 *  1. Delete the created hardlink
 */
static void create_longname255_hardlink(void)
{
	int rc = 0;
	char *long_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901";
	efs_ino_t file_inode = 0LL;

	struct stat stat_out;
	memset(&stat_out, 0, sizeof(stat_out));

	time_t cur_time;
	time(&cur_time);

	ut_assert_int_equal(255, strlen(long_name));

	rc = efs_link(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.file_inode, &ut_efs_obj.current_inode,
			long_name);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, long_name,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj.file_inode, file_inode);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_nlink, 2);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	} 

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, long_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for hardlink creation and deletion of original file
 * Description: Create a hardlink for file and delete file
 * Strategy:
 *  1. Create a file
 *  2. Create hardlink for file
 *  3. Lookup for hardlink
 *  4. Delete created file
 *  5. Lookup for hardlink
 *  6. Verify output
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. Lookup for hardlink should be successful
 * Environment:
 *  1. Delete the created hardlink
 */
static void create_hardlink_delete_original(void)
{
	int rc = 0;
	char *file_name = "test_hardlink_detele_original",
		*link_name = "test_hardlink";

	efs_ino_t file_inode = 0LL, link_inode = 0LL;

	time_t cur_time;

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_link(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &file_inode,
			&ut_efs_obj.current_inode, link_name);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, link_name, &link_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(file_inode, link_inode);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, file_name);

	ut_assert_int_equal(rc, 0);

	time(&cur_time);

	struct stat stat_out;
	memset(&stat_out, 0, sizeof(stat_out));

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &file_inode,
				&stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_nlink, 1);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, link_name);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test for hardlink creation and deletion of link
 * Description: Create a hardlink for file and delete link
 * Strategy:
 *  1. Create hardlink for file
 *  2. Lookup for hardlink
 *  3. Delete created file
 *  4. getattr for file
 *  5. Verify output
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. getattr for file should be successful
 */
static void create_hardlink_delete_link(void)
{
	int rc = 0;
	char *link_name = "test_hardlink";

	time_t cur_time;

	efs_ino_t file_inode = 0LL;

	rc = efs_link(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.file_inode, &ut_efs_obj.current_inode,
			link_name);
	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, link_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj.file_inode, file_inode);

	time(&cur_time);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, link_name);

	ut_assert_int_equal(rc, 0);

	struct stat stat_out;
	memset(&stat_out, '0', sizeof(stat_out));

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/link_ops.log";

	printf("Link Tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = ut_efs_fs_setup();
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	ut_efs_obj.file_name = "test_hardlink_file";

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, ut_efs_obj.file_name, 0755,
			&ut_efs_obj.file_inode);
	if (rc) {
		printf("Failed to create file %s\n", ut_efs_obj.file_name);
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(create_symlink, NULL, NULL),
		ut_test_case(create_longname255_symlink, NULL, NULL),
		ut_test_case(create_hardlink, NULL, NULL),
		ut_test_case(create_longname255_hardlink, NULL, NULL),
		ut_test_case(create_hardlink_delete_original, NULL, NULL),
		ut_test_case(create_hardlink_delete_link, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, ut_efs_obj.file_name);

	if (rc) {
		printf("Failed to create file %s\n", ut_efs_obj.file_name);
	}

	ut_efs_fs_teardown();

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
