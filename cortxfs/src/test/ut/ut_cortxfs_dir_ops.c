/*
 * Filename: ut_cortxfs_dir_ops.c
 * Description: Implementation tests for directory operartions
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

#include "ut_cortxfs_helper.h"
#define DIR_NAME_LEN_MAX 255
#define DIR_ENV_FROM_STATE(__state) (*((struct ut_dir_env **)__state))

struct ut_dir_env {
	struct ut_cfs_params ut_cfs_obj;
	char **name_list;
	int entry_cnt;
};

struct readdir_ctx {
	int index;
	char *readdir_array[256];
};

/**
 * Call-back function for readdir
 */
static bool test_readdir_cb(void *ctx, const char *name,
                            const struct stat *stat)
{
	struct readdir_ctx *readdir_ctx = ctx;

	readdir_ctx->readdir_array[readdir_ctx->index++] = strdup(name);

	return true;
}

/**
 * Free readdir array
 */
static void readdir_ctx_fini(struct readdir_ctx *ctx)
{
	int i;
	for (i = 0; i < ctx->index; i++) {
		free(ctx->readdir_array[i]);
	}
}

/**
 * Verify readdir content
 */
static void verify_dentries(struct readdir_ctx *ctx, struct ut_dir_env *env,
				int entry_start)
{
	int i;
	ut_assert_int_equal(env->entry_cnt, ctx->index);

	for (i = 0; i < ctx->index; i++) {
		ut_assert_string_equal(env->name_list[entry_start + i],
					ctx->readdir_array[i]);
	}
}

/**
 * Setup for reading root directory content
 * Description: Create directory in root directory.
 * Strategy:
 *  1. Create a directory in root directory
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should be successful.
 */
static int readdir_root_dir_setup(void **state)
{
	int rc = 0;
	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_readdir_root_dir";
	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];
	ut_dir_obj->entry_cnt = 1;

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for reading root directory content
 * Description: Read root directory.
 * Strategy:
 *  1. Read root directory
 *  2. Verify readdir content.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The readdir content comparision should not fail.
 */
static void readdir_root_dir(void **state)
{
	int rc = 0;
	cfs_ino_t root_inode = CFS_ROOT_INODE;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_cfs_params *ut_cfs_obj = &ut_dir_obj->ut_cfs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = cfs_readdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred, &root_inode,
				test_readdir_cb, readdir_ctx);
	ut_assert_int_equal(rc, 0);

	verify_dentries(readdir_ctx, ut_dir_obj, 0);

	readdir_ctx_fini(readdir_ctx);
}

/**
 * Teardown for directory realated tests
 * Description: Delete a directory.
 * Strategy:
 * 1. Delete directory.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory deletion should be successful.
 */
static int dir_test_teardown(void **state)
{
	int rc = 0;

	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for reading sub directory content
 * Description: Create directories.
 * Strategy:
 *  1. Create a directory d1 in root directory
 *  2. Create directory d2 inside d1
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should be successful.
 */
static int readdir_sub_dir_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_sub_dir";
	ut_dir_obj->name_list[1] = "test_read_subdir";

	ut_dir_obj->entry_cnt = 1;

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[1];
	ut_dir_obj->ut_cfs_obj.parent_inode = ut_dir_obj->ut_cfs_obj.file_inode;

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for reading sub directory content
 * Description: Read sub directory.
 * Strategy:
 *  3. Read directory d1
 *  4. Verify readdir content.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The readdir content comparision should not fail.
 */
static void readdir_sub_dir(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_cfs_params *ut_cfs_obj = &ut_dir_obj->ut_cfs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_dir_obj->name_list[0],
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = cfs_readdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred, &dir_inode,
				test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(rc, 0);

	verify_dentries(readdir_ctx, ut_dir_obj, 1);

	readdir_ctx_fini(readdir_ctx);
}

/**
 * Teardown for reading sub directory content test
 * Description: Delete directories.
 * Strategy:
 *  1. Delete a directory d1 in root directory
 *  2. Delete directory d2 inside d1
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory deletion should be successful.
 */
static int readdir_sub_dir_teardown(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[1];
	rc = ut_dir_delete(state);
	assert_int_equal(rc, 0);

	ut_dir_obj->ut_cfs_obj.parent_inode = CFS_ROOT_INODE;
	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];

	rc = ut_dir_delete(state);
	assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for reading directory content test
 * Description: Create file and directory
 * Strategy:
 *  1. Create a directory d1 in root directory
 *  3. Create file f1 in root directory
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. File and directory creation should be successful
 */
static int readdir_file_and_dir_setup(void **state)
{
	int rc = 0;
	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_dir";
	ut_dir_obj->name_list[1] = "test_read_file";

	ut_dir_obj->entry_cnt = 2;
	ut_dir_obj->ut_cfs_obj.parent_inode = CFS_ROOT_INODE;

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_file_create(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[1];
	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for reading directory content
 * Description: Read directory.
 * Strategy:
 *  1. Read root directory
 *  2. Verify readdir content.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The readdir content comparision should not fail.
 */
static void readdir_file_and_dir(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_cfs_params *ut_cfs_obj = &ut_dir_obj->ut_cfs_obj; 

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = cfs_readdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(rc, 0);

	verify_dentries(readdir_ctx, ut_dir_obj, 0);

	readdir_ctx_fini(readdir_ctx);
}

/**
 * Teardown for reading directory content test
 * Description: Delete file and directory
 * Strategy:
 *  1. Delete a directory d1 in root directory
 *  2. Delete file f1 in root directory
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. File and directory creation should be successful
 */
static int readdir_file_and_dir_teardown(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_file_delete(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[1];
	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for reading empty directory content
 * Description: Create directory.
 * Strategy:
 *  1. Create a directory d1 in root directory
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should be succssful
 */
static int readdir_empty_dir_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_empty_dir";
	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];

	ut_dir_obj->entry_cnt = 0;

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for reading empty directory content
 * Description: Read empty directory.
 * Strategy:
 *  1. Read d1 directory
 *  2. Verify readdir content to be nothing.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The readdir content should be empty.
 */
static void readdir_empty_dir(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_cfs_params *ut_cfs_obj = &ut_dir_obj->ut_cfs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_dir_obj->name_list[0],
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = cfs_readdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred, &dir_inode,
				test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(rc, 0);

	verify_dentries(readdir_ctx, ut_dir_obj, 0);

	readdir_ctx_fini(readdir_ctx);
}

/**
 * Setup for reading directory which contains multpile directories
 * Description: Create directories.
 * Strategy:
 *  1. Create a directory d0 in root directory
 *  2. Create directories d1 to d8 in d0
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should be succssful.
 */
static int readdir_multiple_dir_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_dir";
	ut_dir_obj->name_list[1] = "dir1";
	ut_dir_obj->name_list[2] = "dir2";
	ut_dir_obj->name_list[3] = "dir3";
	ut_dir_obj->name_list[4] = "dir4";
	ut_dir_obj->name_list[5] = "dir5";
	ut_dir_obj->name_list[6] = "dir6";
	ut_dir_obj->name_list[7] = "dir7";
	ut_dir_obj->name_list[8] = "dir8";

	 ut_dir_obj->entry_cnt = 8;
	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_cfs_obj.parent_inode = ut_dir_obj->ut_cfs_obj.file_inode;

	int i;
	for (i = 0; i<8; i ++) {

		ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[i+1];
		rc = ut_dir_create(state);

		ut_assert_int_equal(rc, 0);
	}

	return rc;
}

/**
 * Test for reading directory which contains multpile directories
 * Description: Read nonempty directory.
 * Strategy:
 *  1. Read directory d0.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Readdir content should match
 */
static void readdir_multiple_dir(void **state)
{
	int rc = 0;

	cfs_ino_t dir_inode = 0LL;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_cfs_params *ut_cfs_obj = &ut_dir_obj->ut_cfs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_dir_obj->name_list[0],
			&dir_inode);
	ut_assert_int_equal(rc, 0);

	rc = cfs_readdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred, &dir_inode,
				test_readdir_cb, readdir_ctx);

	ut_assert_int_equal(rc, 0);

	verify_dentries(readdir_ctx, ut_dir_obj, 1);

	readdir_ctx_fini(readdir_ctx);
}

/**
 * Teardown for reading directory which contains multpile directories
 * Description: Create directories.
 * Strategy:
 *  1. Delete directories d1 to d8 in d0
 *  2. Delete a directory d0 in root directory
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory deletion should be succssful.
 */
static int readdir_multiple_dir_teardown(void **state)
{
	int rc = 0, i;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	for (i = 1; i<9; i ++) {

		ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[i];
		rc = ut_dir_delete(state);
		assert_int_equal(rc, 0);
	}

	ut_dir_obj->ut_cfs_obj.parent_inode = CFS_ROOT_INODE;
	ut_dir_obj->ut_cfs_obj.file_name = ut_dir_obj->name_list[0];

	rc = ut_dir_delete(state);
	assert_int_equal(rc, 0);

	return rc;
}

/**
 * Setup for directory creation test.
 * Description: check if directory exists.
 * Strategy:
 *  1. Check if directory exists
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The lookup should fail with error -ENOENT
 */
static int create_dir_setup(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);
	
	ut_cfs_obj->parent_inode = CFS_ROOT_INODE;
	ut_cfs_obj->file_name = "test_dir";

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, -ENOENT);
	rc = 0;

	return rc;
}

/**
 * Test for directory creation.
 * Description: create a directory.
 * Strategy:
 *  1. Create a directory.
 *  2. Lookup for created directory.
 *  3. Verify output.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The lookup should verify directory creation.
 */
static void create_dir(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	cfs_ino_t dir_inode = 0LL;

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_cfs_obj->file_name, 0755,
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	dir_inode = 0LL;

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, 0);
}

/**
 * Setup for existing directory creation.
 * Description: create a existing directory.
 * Strategy:
 *  1. Check if directory exists
 *  2. Create a directory.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should be successful.
 */
static int create_exist_dir_setup(void **state)
{
	int rc = 0;
	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	ut_cfs_obj->file_name = "test_exist_dir";

	cfs_ino_t dir_inode = 0LL;

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_cfs_obj->file_name, 0755,
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for existing directory creation.
 * Description: create a existing directory.
 * Strategy:
 *  3. Create existing directory.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should fail with error directory exists.
 */
static void create_exist_dir(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_cfs_obj->file_name, 0755,
			&dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Setup for longnamedirectory creation test.
 * Description: check if directory exists.
 * Strategy:
 *  1. Check if directory exists
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The lookup should fail with error -ENOENT
 */
static int create_longname255_dir_setup(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	ut_cfs_obj->file_name = "123456789012345678901234567890123456789012345"
			"67890123456789012345678901234567890123456789012345678"
			"90123456789012345678901234567890123456789012345678901"
			"23456789012345678901234567890123456789012345678901234"
			"567890123456789012345678901234567890123456789012345";

	ut_assert_int_equal(255, strlen(ut_cfs_obj->file_name));

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, -ENOENT);
	rc = 0;

	return rc;
}

/**
 * Test for longname directory creation.
 * Description: create a longname directory.
 * Strategy:
 *  1. Create a longname directory.
 *  2. Lookup for created directory.
 *  3. Verify output.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Lookup should verify directory creation
 */
static void create_longname255_dir(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_cfs_obj->file_name,
			0755, &dir_inode);
	ut_assert_int_equal(rc, 0);

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, 0);
}


/**
 * Setup for longnamedirectory creation test.
 * Description: check if directory exists.
 * Strategy:
 *  1. Check if directory exists
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. The lookup should fail with error -ENOENT
 */
static int create_name_exceed_limit_dir_setup(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	ut_cfs_obj->file_name = "123456789012345678901234567890123456789012345"
			"67890123456789012345678901234567890123456789012345678"
			"90123456789012345678901234567890123456789012345678901"
			"23456789012345678901234567890123456789012345678901234"
			"5678901234567890123456789012345678901234567890123456";

	if(strlen(ut_cfs_obj->file_name) <= DIR_NAME_LEN_MAX) {
		ut_assert_true(0);
	}

	rc = cfs_lookup(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->parent_inode, ut_cfs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, -ENOENT);
	rc = 0;

	return rc;
}

/**
 * Test for longname directory creation.
 * Description: create a longname directory.
 * Strategy:
 *  1. Create a longname directory.
 *  2. Lookup for created directory.
 *  3. Verify output.
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Lookup should verify directory creation
 */
static void create_name_exceed_limit_dir(void **state)
{
	int rc = 0;
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, ut_cfs_obj->file_name,
			0755, &dir_inode);
	ut_assert_int_equal(rc, -E2BIG);
}

/**
 * Test for current directory creation.
 * Description: Test mkdir fails with EXIST when the dentry name is ".".
 * Strategy:
 *  1. Create a directory with dentry name ".".
 * Expected behavior:
 *  1. No errors from CORTXFS API.
 *  2. Directory creation should fail with error directory exists
 */
static void create_current_dir(void **state)
{
	int rc = 0;
	char *dir_name = ".";
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Test for parent directory creation.
 * Description: Test if mkdir fails with EXIST when the dentry name is "..".
 * Strategy:
 *  1. Create directory with dentry name "..".
 * Expected behavior:
 *  1. No errors from CORTXFS API
 *  2. Directory creation should fail with error directory exists
 */
static void create_parent_dir(void **state)
{
	int rc = 0;
	char *dir_name = "..";
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Test for root directory creation.
 * Description: create root directory.
 * Strategy:
 *  1. Create a root directory.
 * Expected behavior:
 *  1. No errors from CORTXFS API
 *  2. Directory creation should fail with error directory exists
 */
static void create_root_dir(void **state)
{
	int rc = 0;
	char *dir_name = "/";
	cfs_ino_t dir_inode = 0LL;

	struct ut_cfs_params *ut_cfs_obj = ENV_FROM_STATE(state);

	rc = cfs_mkdir(ut_cfs_obj->cfs_fs, &ut_cfs_obj->cred,
			&ut_cfs_obj->current_inode, dir_name, 0755, &dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Setup for dir_ops test group
 */
static int dir_ops_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = calloc(sizeof(struct ut_dir_env), 1);

	ut_assert_not_null(ut_dir_obj);

	ut_dir_obj->name_list = calloc(sizeof(char *), 10);

	if(ut_dir_obj->name_list == NULL) {
		rc = -ENOMEM;
		ut_assert_true(0);
	}

	*state = ut_dir_obj;
	rc = ut_cfs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for dir_ops test group
 */
static int dir_ops_teardown(void **state)
{
	int rc = 0;

	rc = ut_cfs_fs_teardown(state);
	ut_assert_int_equal(rc, 0);
	
	free(*state);

	return rc;
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/ut_cortxfs.log";

	printf("Directory tests\n");

	rc = ut_load_config(CONF_FILE);
	if (rc != 0) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_log = ut_get_config("cortxfs", "log_path", test_log);

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		goto out;
	}

	struct test_case test_list[] = {
		ut_test_case(readdir_root_dir, readdir_root_dir_setup,
				dir_test_teardown),
		ut_test_case(readdir_file_and_dir, readdir_file_and_dir_setup,
				readdir_file_and_dir_teardown),
		ut_test_case(readdir_sub_dir, readdir_sub_dir_setup,
				readdir_sub_dir_teardown),
		ut_test_case(readdir_empty_dir,readdir_empty_dir_setup,
				dir_test_teardown),
		ut_test_case(readdir_multiple_dir, readdir_multiple_dir_setup,
				readdir_multiple_dir_teardown),
		ut_test_case(create_dir, create_dir_setup, dir_test_teardown),
		ut_test_case(create_exist_dir, create_exist_dir_setup,
				dir_test_teardown),
		ut_test_case(create_longname255_dir,
				create_longname255_dir_setup, dir_test_teardown),
		ut_test_case(create_name_exceed_limit_dir,
				create_name_exceed_limit_dir_setup, NULL),
		ut_test_case(create_current_dir, NULL, NULL),
		ut_test_case(create_parent_dir, NULL, NULL),
		ut_test_case(create_root_dir, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, dir_ops_setup,
				dir_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
