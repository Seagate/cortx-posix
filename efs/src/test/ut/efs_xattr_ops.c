/*
 * Filename: efs_xattr_ops.c
 * Description: Implementation tests for xattr
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
#include <sys/xattr.h> /* XATTR_CREATE */
#define XATTR_VAL_SIZE_MAX 4096

struct ut_efs_params ut_efs_obj;

/**
 * Test to set new (non-existing) xattr
 * Description: Set new (non-existing) xattr for file.
 * Strategy:
 *  1. Set xattr
 *  2. Verify setting xattr by getting xattr value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. xattr recieved from getxattr should match set xattr.
 */
static void set_nonexist_xattr(void)
{
	int rc = 0;

	char *xattr_name = "set.nonexist.xattr", *xattr_val = "1234567890";
	size_t val_size = strlen(xattr_val), get_val_size = XATTR_VAL_SIZE_MAX;

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, xattr_val,
				val_size, XATTR_CREATE);

	ut_assert_int_equal(rc, 0);

	char buf[XATTR_VAL_SIZE_MAX] = {0};

	rc = efs_getxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, buf,
				&get_val_size);

	ut_assert_int_equal(rc, 0);
	ut_assert_int_equal(get_val_size, val_size);

	rc = memcmp(xattr_val, buf, strlen(xattr_val));

	ut_assert_int_equal(rc, 0);
}

/**
 * Test to set existing xattr
 * Description: Set existing xattr for file.
 * Strategy:
 *  1. Set xattr
 *  2. Set same xattr again
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Second setxattr operation should fail with error EEXIST
 */
static void set_exist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "set.exist.xattr", *xattr_val = "1234567890",
		*xattr_new_val = strdup(xattr_val);
	size_t val_size = strlen(xattr_val);

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, xattr_val,
				val_size, XATTR_CREATE);

	ut_assert_int_equal(rc, 0);

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name,
				xattr_new_val, val_size, XATTR_CREATE);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Test to replace new (non-existing) xattr
 * Description: Replace new (non-existing) xattr for file.
 * Strategy:
 *  1. Set new (non-existing) xattr with flag XATTR_REPLACE
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2.Setxattr operation should fail with error ENOENT
 */
static void replace_nonexist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "replace.nonexist.xattr", *xattr_val = "123456789";
	size_t val_size = strlen(xattr_val);

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, xattr_val,
				val_size, XATTR_REPLACE);

	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to replace existing xattr
 * Description: Replace existing xattr for file.
 * Strategy:
 *  1. Set xattr
 *  2. Verify setting xattr by getting xattr value
 *  3. Replace same xattr again
 *  4. Verify replace xattr by getting xattr value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Second setxattr operation with flag XATTR_REPLACE should be successful
 */
static void replace_exist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "replace.exist.xattr", *xattr_val = "1234567890",
		 *xattr_new_val = strdup(xattr_val);

	size_t val_size = strlen(xattr_val), get_val_size = XATTR_VAL_SIZE_MAX;

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, xattr_val,
				val_size, XATTR_CREATE);

	ut_assert_int_equal(rc, 0);

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name,
				xattr_new_val, val_size, XATTR_REPLACE);

	ut_assert_int_equal(rc, 0);

	char buf[XATTR_VAL_SIZE_MAX] = {0};

	rc = efs_getxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, buf,
				&get_val_size);

	ut_assert_int_equal(rc, 0);
	ut_assert_int_equal(get_val_size, val_size);

	rc = memcmp(xattr_new_val, buf, strlen(xattr_new_val));

	ut_assert_int_equal(rc, 0);
}

/**
 * Test to get existing xattr
 * Description: Get existing xattr for file.
 * Strategy:
 *  1. Set xattr
 *  2. Get xattr value
 *  3. Verify output
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. value from getxattr should match with set value.
 */
static void get_exist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "get.exist.xattr", *xattr_set_val = "1234567890";
	size_t xattr_set_size = strlen(xattr_set_val);

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name,
				xattr_set_val, xattr_set_size, XATTR_CREATE);

	ut_assert_int_equal(rc, 0);

	char xattr_val[XATTR_VAL_SIZE_MAX] = {0};
	size_t val_size = XATTR_VAL_SIZE_MAX;

	rc  = efs_getxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
		           &ut_efs_obj.file_inode, xattr_name, xattr_val,
		           &val_size);

	ut_assert_int_equal(rc, 0);
	ut_assert_int_equal(val_size, xattr_set_size);

	rc = memcmp(xattr_set_val, xattr_val, xattr_set_size);

	ut_assert_int_equal(rc, 0);
}

/**
 * Test to get nonexisting xattr
 * Description: Get nonexisting xattr for file.
 * Strategy:
 *  1. Get xattr value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. getxattr soperation should fail with error ENOENT
 */
static void get_nonexist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "get.nonexist.xattr";

	char xattr_val[XATTR_VAL_SIZE_MAX] = {0};
	size_t val_size = XATTR_VAL_SIZE_MAX;

	rc = efs_getxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, xattr_val,
				&val_size);

	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to remove existing xattr
 * Description: Remove existing xattr for file.
 * Strategy:
 *  1. Set xattr
 *  2. Remove xattr
 *  3. Verify xattr remove operation by getting xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. xattr remove operation should be successful
 *  3. getxattr should fail with error ENOENT.
 */
static void remove_exist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "remove.exist.xattr", *xattr_set_val = "1234567890";
	size_t xattr_set_size = strlen(xattr_set_val);

	rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name,
				xattr_set_val, xattr_set_size, XATTR_CREATE);
	ut_assert_int_equal(rc, 0);

	rc = efs_removexattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name);

	ut_assert_int_equal(rc, 0);

	char xattr_val[XATTR_VAL_SIZE_MAX] = {0};
	size_t val_size = XATTR_VAL_SIZE_MAX;

	rc = efs_getxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name, xattr_val,
				&val_size);

	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to remove nonexisting xattr
 * Description: Remove nonexisting xattr for file.
 * Strategy:
 *  1. Remove xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. xattr remove operation should fail with error ENOENT
 */
static void remove_nonexist_xattr(void)
{
	int rc = 0;
	char *xattr_name = "remove.nonexist.xattr";

	rc = efs_removexattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, xattr_name);

	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to list xattr
 * Description: List xattr for file.
 * Strategy:
 *  1. Set multiple xattr
 *  2. List xattr
 *  3. xattrs values received from list xattr should match with set values
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. list xattr operation should be successful
 *  3. xattr values set and recieved from list xattr should match.
 */
static void listxattr_test(void)
{
	int rc = 0;
	char xattr_name[][4096] = { "xattr.checksum", "xattr.attribute"},
		xattr_set_val[][XATTR_VAL_SIZE_MAX] = {"1234567890", "999999999"},
		*file_name = "test.listxattr";
	size_t xattr_set_size[] = {strlen(xattr_set_val[0]),
					strlen(xattr_set_val[1])};
	efs_ino_t file_inode = 0LL;

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, file_name, 0755, &file_inode);

	ut_assert_int_equal(rc, 0);

	int i, xattr_set_cnt = sizeof(xattr_set_size)/sizeof(xattr_set_size[0]);

	for (i = 0; i<xattr_set_cnt; i ++) {
		rc = efs_setxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
					&file_inode, xattr_name[i],
					xattr_set_val[i], xattr_set_size[i],
					XATTR_CREATE);

		ut_assert_int_equal(rc, 0);
	}

	size_t count, buf_size = XATTR_VAL_SIZE_MAX;
	char *buf;

	buf = malloc(buf_size * sizeof(*buf));

	rc = efs_listxattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred, &file_inode,
				buf, &count, &buf_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(xattr_set_cnt, count);

	char *p = buf;
	int len;

	for (i = 0; i<count; i++) {
		len = strlen(xattr_name[i]);
		rc = memcmp(p, xattr_name[i], len);

		ut_assert_int_equal(rc, 0);

		p += len;
		p++;
	}

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, file_name);
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/xattr_ops.log";

	printf("Xattr Tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = ut_efs_fs_setup();
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	ut_efs_obj.file_name = "test_xattr_file";

	ut_efs_obj.file_inode = 0LL;

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, ut_efs_obj.file_name, 0755,
			&ut_efs_obj.file_inode);
	if (rc) {
		printf("Failed to create file %s\n", ut_efs_obj.file_name);
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(set_exist_xattr, NULL, NULL),
		ut_test_case(set_nonexist_xattr, NULL, NULL),
		ut_test_case(replace_exist_xattr, NULL, NULL),
		ut_test_case(replace_nonexist_xattr, NULL, NULL),
		ut_test_case(get_exist_xattr, NULL, NULL),
		ut_test_case(get_nonexist_xattr, NULL, NULL),
		ut_test_case(remove_exist_xattr, NULL, NULL),
		ut_test_case(remove_nonexist_xattr, NULL, NULL),
		ut_test_case(listxattr_test, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, ut_efs_obj.file_name);

	ut_efs_fs_teardown();

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
