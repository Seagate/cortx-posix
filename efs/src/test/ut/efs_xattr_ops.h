/*
 * Filename: efs_xattr_ops.h
 * Description: Declaration of ops for xattr file and directory tests
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
 */

#include "ut_efs_helper.h"
#include <sys/xattr.h> /* XATTR_CREATE */

#define XATTR_VAL_SIZE_MAX 4096
#define XATTR_ENV_FROM_STATE(__state) (*((struct ut_xattr_env **)__state))

struct ut_xattr_env {
	struct ut_efs_params ut_efs_obj;
	char **xattr;
};

/**
 * Test to set new (non-existing) xattr
 * Description: Set new (non-existing) xattr for file or directory
 * Strategy:
 *  1. Set xattr
 *  2. Verify setting xattr by getting xattr value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. xattr recieved from getxattr should match set xattr.
 */
void set_nonexist_xattr(void **state);

/**
 * Setup for set existing xattr test
 * Description: Set new xattr for file or directory.
 * Strategy:
 *  1. Set xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. set xattr operation should be successful
 */
int set_exist_xattr_setup(void **state);

/**
 * Test to set existing xattr
 * Description: Set existing xattr for file or directory.
 * Strategy:
 *  1. Set existing xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. setxattr operation should fail with error EEXIST
 */
void set_exist_xattr(void **state);

/**
 * Test to replace new (non-existing) xattr
 * Description: Replace new (non-existing) xattr for file or directory.
 * Strategy:
 *  1. Set new (non-existing) xattr with flag XATTR_REPLACE
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Setxattr operation should fail with error ENOENT
 */
void replace_nonexist_xattr(void **state);

/**
 * Setup for replace existing xattr test
 * Description: Set new xattr for file or directory.
 * Strategy:
 *  1. Set xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. setxattr operation should be successful
 */
int replace_exist_xattr_setup(void **state);

/**
 * Test to replace existing xattr
 * Description: Replace existing xattr for file or directory.
 * Strategy:
 *  1. Replace existing xattr
 *  2. Verify replace xattr by getting xattr value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. setxattr operation with flag XATTR_REPLACE should be successful
 */
void replace_exist_xattr(void **state);

/**
 * Setup for get existing xattr test
 * Description: Set xattr for file or directory.
 * Strategy:
 *  1. Set xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Setxattr operation should be successful
 */
int get_exist_xattr_setup(void **state);

/**
 * Test to get existing xattr
 * Description: Get existing xattr for file or directory.
 * Strategy:
 *  1. Get xattr value
 *  2. Verify output
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. value from getxattr should match with set value.
 */
void get_exist_xattr(void **state);

/**
 * Test to get nonexisting xattr
 * Description: Get nonexisting xattr for file or directory.
 * Strategy:
 *  1. Get xattr value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. getxattr soperation should fail with error ENOENT
 */
void get_nonexist_xattr(void **state);

/**
 * Setup for remove existing xattr test
 * Description: Set xattr for file or directory.
 * Strategy:
 *  1. Set xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Setxattr operation should be successful.
 */
int remove_exist_xattr_setup(void **state);

/**
 * Test to remove existing xattr
 * Description: Remove existing xattr for file or directory.
 * Strategy:
 *  1. Remove xattr
 *  2. Verify xattr remove operation by getting xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. xattr remove operation should be successful
 *  3. getxattr should fail with error ENOENT.
 */
void remove_exist_xattr(void **state);

/**
 * Test to remove nonexisting xattr
 * Description: Remove nonexisting xattr for file or directory.
 * Strategy:
 *  1. Remove xattr
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. xattr remove operation should fail with error ENOENT
 */
void remove_nonexist_xattr(void **state);

/**
 * Setup for list xattr test
 * Description: Set xattrs for file or directory.
 * Strategy:
 *  1. Create a file or directory.
 *  2. Set multiple xattr for file or directory.
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Setxattr operations should be successful.
 */
int listxattr_test_setup(void **state);

/**
 * Test to list xattr
 * Description: List xattr for file or directory.
 * Strategy:
 *  1. List xattr
 *  2. xattrs values received from list xattr should match with set values
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. list xattr operation should be successful
 *  3. xattr values set and recieved from list xattr should match.
 */
void listxattr_test(void **state);

/**
 * Teardown for list xattr test
 * Description: Delete file or directory.
 * Strategy:
 * 1. Delete file or directory
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. File deletion should be successful.
 */
int listxattr_test_teardown(void **state);
