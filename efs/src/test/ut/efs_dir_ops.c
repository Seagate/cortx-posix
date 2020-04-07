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
#define DIR_NAME_LEN_MAX 255
#define DIR_ENV_FROM_STATE(__state) (*((struct ut_dir_env **)__state))

struct ut_dir_env {
	struct ut_efs_params ut_efs_obj;
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
static bool test_readdir_cb(void *ctx, const char *name, const efs_ino_t *ino)
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
 *  1. No errors from EFS API.
 *  2. Directory creation should be successful.
 */
static int readdir_root_dir_setup(void **state)
{
	int rc = 0;
	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_readdir_root_dir";
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];
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
 *  1. No errors from EFS API.
 *  2. The readdir content comparision should not fail.
 */
static void readdir_root_dir(void **state)
{
	int rc = 0;
	efs_ino_t root_inode = EFS_ROOT_INODE;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_dir_obj->ut_efs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_readdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &root_inode,
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
 *  1. No errors from EFS API.
 *  2. Directory deletion should be successful.
 */
static int dir_test_teardown(void **state)
{
	int rc = 0;

	rc = ut_file_delete(state);
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
 *  1. No errors from EFS API.
 *  2. Directory creation should be successful.
 */
static int readdir_sub_dir_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_sub_dir";
	ut_dir_obj->name_list[1] = "test_read_subdir";

	ut_dir_obj->entry_cnt = 1;

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[1];
	ut_dir_obj->ut_efs_obj.parent_inode = ut_dir_obj->ut_efs_obj.file_inode;

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
 *  1. No errors from EFS API.
 *  2. The readdir content comparision should not fail.
 */
static void readdir_sub_dir(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_dir_obj->ut_efs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_dir_obj->name_list[0],
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &dir_inode,
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
 *  1. No errors from EFS API.
 *  2. Directory deletion should be successful.
 */
static int readdir_sub_dir_teardown(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[1];
	rc = ut_dir_delete(state);
	assert_int_equal(rc, 0);

	ut_dir_obj->ut_efs_obj.parent_inode = EFS_ROOT_INODE;
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];

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
 *  1. No errors from EFS API.
 *  2. File and directory creation should be successful
 */
static int readdir_file_and_dir_setup(void **state)
{
	int rc = 0;
	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_dir";
	ut_dir_obj->name_list[1] = "test_read_file";

	ut_dir_obj->entry_cnt = 2;
	ut_dir_obj->ut_efs_obj.parent_inode = EFS_ROOT_INODE;

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_file_create(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[1];
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
 *  1. No errors from EFS API.
 *  2. The readdir content comparision should not fail.
 */
static void readdir_file_and_dir(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_dir_obj->ut_efs_obj; 

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_readdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, test_readdir_cb, readdir_ctx);

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
 *  1. No errors from EFS API.
 *  2. File and directory creation should be successful
 */
static int readdir_file_and_dir_teardown(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_file_delete(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[1];
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
 *  1. No errors from EFS API.
 *  2. Directory creation should be succssful
 */
static int readdir_empty_dir_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->name_list[0] = "test_read_empty_dir";
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];

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
 *  1. No errors from EFS API.
 *  2. The readdir content should be empty.
 */
static void readdir_empty_dir(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_dir_obj->ut_efs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_dir_obj->name_list[0],
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &dir_inode,
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
 *  1. No errors from EFS API.
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
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];

	rc = ut_dir_create(state);
	ut_assert_int_equal(rc, 0);

	ut_dir_obj->ut_efs_obj.parent_inode = ut_dir_obj->ut_efs_obj.file_inode;

	int i;
	for (i = 0; i<8; i ++) {

		ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[i+1];
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
 *  1. No errors from EFS API.
 *  2. Readdir content should match
 */
static void readdir_multiple_dir(void **state)
{
	int rc = 0;

	efs_ino_t dir_inode = 0LL;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_dir_obj->ut_efs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_dir_obj->name_list[0],
			&dir_inode);
	ut_assert_int_equal(rc, 0);

	rc = efs_readdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &dir_inode,
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
 *  1. No errors from EFS API.
 *  2. Directory deletion should be succssful.
 */
static int readdir_multiple_dir_teardown(void **state)
{
	int rc = 0, i;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	for (i = 1; i<9; i ++) {

		ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[i];
		rc = ut_dir_delete(state);
		assert_int_equal(rc, 0);
	}

	ut_dir_obj->ut_efs_obj.parent_inode = EFS_ROOT_INODE;
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];

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
 *  1. No errors from EFS API.
 *  2. The lookup should fail with error -ENOENT
 */
static int create_dir_setup(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);
	
	ut_efs_obj->parent_inode = EFS_ROOT_INODE;
	ut_efs_obj->file_name = "test_dir";

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
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
 *  1. No errors from EFS API.
 *  2. The lookup should verify directory creation.
 */
static void create_dir(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	efs_ino_t dir_inode = 0LL;

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name, 0755,
			&dir_inode);

	ut_assert_int_equal(rc, 0);

	dir_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
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
 *  1. No errors from EFS API.
 *  2. Directory creation should be successful.
 */
static int create_exist_dir_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->file_name = "test_exist_dir";

	efs_ino_t dir_inode = 0LL;

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, -ENOENT);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name, 0755,
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
 *  1. No errors from EFS API.
 *  2. Directory creation should fail with error directory exists.
 */
static void create_exist_dir(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name, 0755,
			&dir_inode);

	ut_assert_int_equal(rc, -EEXIST);
}

/**
 * Setup for longnamedirectory creation test.
 * Description: check if directory exists.
 * Strategy:
 *  1. Check if directory exists
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should fail with error -ENOENT
 */
static int create_longname255_dir_setup(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->file_name = "123456789012345678901234567890123456789012345"
			"67890123456789012345678901234567890123456789012345678"
			"90123456789012345678901234567890123456789012345678901"
			"23456789012345678901234567890123456789012345678901234"
			"567890123456789012345678901234567890123456789012345";

	ut_assert_int_equal(255, strlen(ut_efs_obj->file_name));

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
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
 *  1. No errors from EFS API.
 *  2. Lookup should verify directory creation
 */
static void create_longname255_dir(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name,
			0755, &dir_inode);
printf("rc == %d\n", rc);
	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, 0);
}


/**
 * Setup for longnamedirectory creation test.
 * Description: check if directory exists.
 * Strategy:
 *  1. Check if directory exists
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. The lookup should fail with error -ENOENT
 */
static int create_name_exceed_limit_dir_setup(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->file_name = "123456789012345678901234567890123456789012345"
			"67890123456789012345678901234567890123456789012345678"
			"90123456789012345678901234567890123456789012345678901"
			"23456789012345678901234567890123456789012345678901234"
			"5678901234567890123456789012345678901234567890123456";

	if(strlen(ut_efs_obj->file_name) <= DIR_NAME_LEN_MAX) {
		ut_assert_true(0);
	}

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
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
 *  1. No errors from EFS API.
 *  2. Lookup should verify directory creation
 */
static void create_name_exceed_limit_dir(void **state)
{
	int rc = 0;
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, ut_efs_obj->file_name,
			0755, &dir_inode);
printf("rc == %d\n", rc);
	ut_assert_int_equal(rc, -E2BIG);
/*	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->parent_inode, ut_efs_obj->file_name,
			&dir_inode);

	ut_assert_int_equal(rc, 0);*/
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
static void create_current_dir(void **state)
{
	int rc = 0;
	char *dir_name = ".";
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, dir_name, 0755, &dir_inode);

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
static void create_parent_dir(void **state)
{
	int rc = 0;
	char *dir_name = "..";
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, dir_name, 0755, &dir_inode);

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
static void create_root_dir(void **state)
{
	int rc = 0;
	char *dir_name = "/";
	efs_ino_t dir_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	rc = efs_mkdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, dir_name, 0755, &dir_inode);

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

	ut_dir_obj->name_list = calloc(sizeof(char *), 3);

	if(ut_dir_obj->name_list == NULL) {
		rc = -ENOMEM;
		ut_assert_true(0);
	}

	*state = ut_dir_obj;
	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for dir_ops test group
 */
static int dir_ops_teardown(void **state)
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
	char *test_log = "/var/log/eos/test/ut/efs/dir_ops.log";

	printf("Directory tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		exit(1);
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

	return 0;
}
