/*
 * Filename: efs_dir_ops.c
 * Description: Implementation tests for directory operartions
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

char readdir_array[256];
int entry_cnt;

struct readdir_ctx {
	int index;
};

/**
 * Call-back function for readdir
 */
static bool test_readdir_cb(void *ctx, const char *name, const efs_ino_t *ino)
{
	struct readdir_ctx *readdir_ctx = ctx;

	memcpy(readdir_array + strlen(readdir_array), name, strlen(name));

	entry_cnt ++;
	readdir_ctx->index++;

	return true;
}

/**
 * Test for reading root directory content
 * Description: Read root directory.
 * Strategy:
 *  1. Create a directory in root directory
 *  2. Read root directory
 *  3. Verify readdir content.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The readdir content comparision should not fail.
 * Environment:
 *  1. Delete created directory.
 */
static void readdir_root_dir(void)
{
	int rc = 0;
	char *dir_name = "test_readdir_root_dir";
	efs_ino_t root_inode = EFS_ROOT_INODE, dir_inode = 0LL;

	memset(readdir_array, '\0', 256);
	entry_cnt = 0;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &root_inode,
			dir_name, 0755, &dir_inode);
	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &root_inode,
				test_readdir_cb, readdir_ctx);
	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(entry_cnt, 1);

	rc = memcmp(dir_name, readdir_array, strlen(dir_name));

	ut_assert_int_equal(rc, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &root_inode,
			dir_name);

	assert_int_equal(rc, 0);
}

/**
 * Test for reading sub directory content
 * Description: Read sub directory.
 * Strategy:
 *  1. Create a directory d1 in root directory
 *  2. Create directory d2 inside d1
 *  3. Read root directory d1
 *  4. Verify readdir content.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The readdir content comparision should not fail.
 * Environment:
 *  1. Delete directory d2.
 *  2. Delete directory d1.
 */
static void readdir_sub_dir(void)
{
	int rc = 0;
	char *sub_dir = "test_read_sub_dir",
		*inner_dir_name = "test_readdir_subdir";
	efs_ino_t sub_dir_inode = 0LL, inner_dir_inode = 0LL;

	memset(readdir_array, '\0', 256);
	entry_cnt = 0;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, sub_dir, 0755, &sub_dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &sub_dir_inode,
			inner_dir_name, 0755, &inner_dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &sub_dir_inode,
				test_readdir_cb, readdir_ctx);
	ut_assert_int_equal(entry_cnt, 1);

	rc = memcmp(inner_dir_name, readdir_array, strlen(inner_dir_name));

	ut_assert_int_equal(rc, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &sub_dir_inode,
			inner_dir_name);

	assert_int_equal(rc, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, sub_dir);

	assert_int_equal(rc, 0);
}

/**
 * Test for reading directory content
 * Description: Read directory.
 * Strategy:
 *  1. Create a directory d1 in root directory
 *  2. Create directory d2 inside d1
 *  3. Create file f1 inside d1
 *  4. Read d1 directory
 *  5. Verify readdir content.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The readdir content comparision should not fail.
 * Environment:
 *  1. Delete created directory d2.
 *  2. Delete created file f1.
 *  3. Delete created directory d1.
 */
static void readdir_file_and_dir(void)
{
	int rc = 0;

	char * file_dir_name[] = {"test_read_file",
				"test_read_dir"};
	efs_ino_t dir_inode = 0LL, file_inode = 0LL;

	memset(readdir_array, '\0', 256);
	entry_cnt = 0;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_creat(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_dir_name[0], 0755,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_dir_name[1], 0755,
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(entry_cnt, 2);

	int i;
	char * buf = readdir_array;
	for (i = 1;i >= 0;i--) {
		rc = memcmp(file_dir_name[i], buf, strlen(file_dir_name[i]));
		buf += strlen(file_dir_name[i]);
		ut_assert_int_equal(rc, 0);
	}

	rc = efs_unlink(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, NULL, file_dir_name[0]);

	assert_int_equal(rc, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, file_dir_name[1]);

	assert_int_equal(rc, 0);
}

/**
 * Test for reading empty directory content
 * Description: Read empty directory.
 * Strategy:
 *  1. Create a directory d1 in root directory
 *  2. Read d1 directory
 *  3. Verify readdir content to be nothing.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The readdir content should be empty.
 *  Environment:
 *   1. Delete created directory d1.
 */
static void readdir_empty_dir(void)
{
	int rc = 0;
	char *dir_name = "test_read_empty_dir";
	efs_ino_t dir_inode = 0LL;

	memset(readdir_array, '\0', 256);
	entry_cnt = 0;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &dir_inode,
				test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(entry_cnt, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name);

	assert_int_equal(rc, 0);
}

/**
 * Test for reading directory which contains multpile directories
 * Description: Read nonempty directory.
 * Strategy:
 *  1. Create a directory d0 in root directory
 *  2. Create directories d1 to d8 in d0
 *  3. Read d0 directory
 *  4. Verify readdir content to be d1 to d8
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The readdir content should be d1 to d8.
 * Environment:
 *  1. Delete created directories d1 to d8.
 *  2. Delete created directories d0
 */
static void readdir_multiple_dir(void)
{
	int rc = 0;

	char *dir_name = "test_read_dir";
	char *inner_dir_name[] = { "dir1",
				"dir2",
				"dir3",
				"dir4",
				"dir5",
				"dir6",
				"dir7",
				"dir8"};

	efs_ino_t dir_inode = 0LL, inner_dir_inode = 0LL;

	memset(readdir_array, '\0', 256);
	entry_cnt = 0;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, 0);

	int i;
	for (i = 0; i<8; i ++) {

		rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &dir_inode,
				inner_dir_name[i], 0755, &inner_dir_inode);
		ut_assert_int_equal(rc, 0);
	}

	rc = efs_readdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &dir_inode,
				test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(entry_cnt, 8);

	char * buf = readdir_array;
	for (i = 0; i<8; i ++) {
		rc = memcmp(inner_dir_name[i], buf, strlen(inner_dir_name[i]));

		ut_assert_int_equal(rc, 0);

		buf += strlen(inner_dir_name[i]);
	}

	for (i = 0; i<8; i ++) {
		rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred, &dir_inode,
				inner_dir_name[i]);
		assert_int_equal(rc, 0);
	}

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name);

	assert_int_equal(rc, 0);
}

/**
 * Test for directory creation.
 * Description: create a directory.
 * Strategy:
 *  1. Check if directory exists
 *  2. Create a directory.
 *  3. Lookup for created directory.
 *  4. Verify output.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should verify directory creation.
 * Environment:
 *  1.Delete directory during teardown.
 */
static void create_dir(void)
{
	int rc = 0;
	char *dir_name = "test_dir";

	efs_ino_t dir_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name, &dir_inode);

	ut_assert_int_not_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name);

	assert_int_equal(rc, 0);
}

/**
 * Test for existing directory creation.
 * Description: create a existing directory.
 * Strategy:
 *  1. Check if directory exists
 *  2. Create a directory.
 *  2. Verify directory creation with lookup.
 *  3. Create same existing directory.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Second time directory creation should fail with error directory exists.
 * Environment:
 *  1.Delete directory during teardown.
 */
static void create_exist_dir(void)
{
	int rc = 0;
	char *dir_name = "test_exist_dir";

	efs_ino_t dir_inode = 0LL;

	rc = efs_lookup(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name, &dir_inode);

	ut_assert_int_not_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name);

	assert_int_equal(rc, 0);
}

/**
 * Test for longname directory creation.
 * Description: create a longname directory.
 * Strategy:
 *  1. Check if direcory exists.
 *  2. Create a longname directory.
 *  3. Lookup for created directory.
 *  4. Verify output.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Lookup should verify directory creation
 * Environment:
 *  1.Delete directory during teardown.
 */
static void create_longname255_dir(void)
{
	int rc = 0;
	char *dir_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901";
	efs_ino_t dir_inode = 0LL;

	ut_assert_int_equal(255, strlen(dir_name));

	rc = efs_lookup(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name, &dir_inode);

	ut_assert_int_not_equal(rc, 0);

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name, &dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_rmdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.parent_inode, dir_name);

	assert_int_equal(rc, 0);
}

/**
 * Test for current directory creation.
 * Description: Test mkdir fails with EXIST when the dentry name is ".".
 * Strategy:
 *  1. Create a directory with dentry name ".".
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Directory creation should fail with error directory exists
 */
static void create_current_dir(void)
{
	int rc = 0;
	char *dir_name = ".";

	efs_ino_t dir_inode = 0LL;

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Test for parent directory creation.
 * Description: Test if mkdir fails with EXIST when the dentry name is "..".
 * Strategy:
 *  1. Create directory with dentry name "..".
 * Expected behavior:
 *  1. No errors from EFS API
 *  2. Directory creation should fail with error directory exists
 */
void create_parent_dir(void)
{
	int rc = 0;
	char *dir_name = "..";

	efs_ino_t dir_inode = 0LL;

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Test for root directory creation.
 * Description: create root directory.
 * Strategy:
 *  1. Create a root directory.
 * Expected behavior:
 *  1. No errors from EFS API
 *  2. Directory creation should fail with error directory exists
 */
static void create_root_dir(void)
{
	int rc = 0;
	char *dir_name = "/";

	efs_ino_t dir_inode = 0LL;

	rc = efs_mkdir(ut_efs_obj.fs_ctx, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/dir_ops.log";

	printf("Directory tests\n");

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
		ut_test_case(readdir_root_dir),
		ut_test_case(readdir_sub_dir),
		ut_test_case(readdir_empty_dir),
		ut_test_case(readdir_multiple_dir),
		ut_test_case(readdir_file_and_dir),
		ut_test_case(create_dir),
		ut_test_case(create_exist_dir),
		ut_test_case(create_longname255_dir),
		ut_test_case(create_current_dir),
		ut_test_case(create_parent_dir),
		ut_test_case(create_root_dir),
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
