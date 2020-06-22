/*
 * Filename: efs_xattr_dir_ops.c
 * Description: Tests for  xattr operations on directories.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Subhash Arya <subhash.arya@seagate.com>
*/
#include "efs_xattr_ops.h"

/**
 * Setup for xattr directory test group.
 */
static int xattr_dir_ops_setup(void **state)
{
	int rc = 0;
	struct ut_xattr_env *ut_xattr_obj = NULL;

	ut_xattr_obj = calloc(sizeof(struct ut_xattr_env), 1);

	ut_assert_not_null(ut_xattr_obj);

	ut_xattr_obj->xattr = calloc(sizeof(char *), 8);

	if( ut_xattr_obj->xattr == NULL) {
		ut_assert_true(0);
		rc = -ENOMEM;
	}

	*state = ut_xattr_obj;
	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	ut_xattr_obj->ut_efs_obj.file_name = "test_xattr_dir";
	ut_xattr_obj->ut_efs_obj.file_inode = 0LL;

	*state = ut_xattr_obj;
	rc = ut_dir_create(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for xattr directory test group
 */
static int xattr_dir_ops_teardown(void **state)
{
	int rc = 0;

	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc, 0);

	rc = ut_efs_fs_teardown(state);
	ut_assert_int_equal(rc, 0);

	free(*state);

	return rc;
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/xattr_dir_ops.log";

	printf("Xattr dir Tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(set_exist_xattr, set_exist_xattr_setup, NULL),
		ut_test_case(set_nonexist_xattr, NULL, NULL),
		ut_test_case(replace_exist_xattr, replace_exist_xattr_setup, NULL),
		ut_test_case(replace_nonexist_xattr, NULL, NULL),
		ut_test_case(get_exist_xattr, get_exist_xattr_setup, NULL),
		ut_test_case(get_nonexist_xattr, NULL, NULL),
		ut_test_case(remove_exist_xattr, remove_exist_xattr_setup, NULL),
		ut_test_case(remove_nonexist_xattr, NULL, NULL),
		ut_test_case(listxattr_test, listxattr_test_setup,
				listxattr_test_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, xattr_dir_ops_setup,
				xattr_dir_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
