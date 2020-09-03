/*
 * Filename: ut_efs_link_ops.c
 * Description: Implementation tests for links
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
#define LINK_ENV_FROM_STATE(__state) (*((struct ut_link_env **)__state))

struct ut_link_env {
	struct ut_efs_params ut_efs_obj;
	char * link_name;
};

/**
 * Teardown for symlink test
 * Description: delete a symlink with ordinary contents.
 * Strategy:
 *  1. Destroy symlink .
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. Symlink deletion should be successful.
 */
static int symlink_teardown(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->parent_inode = EFS_ROOT_INODE;

	rc = ut_file_delete(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

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
 */
static void create_symlink(void **state)
{
	int rc = 0;
	char *link_name = "test_symlink",
		*symlink_content = "abcdefghijklmnopqrstuvwxyz";

	efs_ino_t link_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);
	ut_efs_obj->file_name = link_name;

	rc = efs_symlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
				&ut_efs_obj->current_inode, link_name,
				symlink_content, &link_inode);

	ut_assert_int_equal(rc, 0);

	char buff[100];
	size_t content_size = 100;

	rc = efs_readlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &link_inode,
				buff, &content_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(content_size, strlen(symlink_content));

	rc = memcmp(buff, symlink_content, strlen(symlink_content));

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
 */
static void create_longname255_symlink(void **state)
{
	int rc = 0;
	char *long_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901",
		*symlink_content = "abcdefghijklmnopqrstuvwxyz";

	efs_ino_t link_inode = 0LL;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);
	ut_efs_obj->file_name = long_name;

	ut_assert_int_equal(255, strlen(long_name));

	rc = efs_symlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
				&ut_efs_obj->current_inode, long_name,
				symlink_content, &link_inode);


	ut_assert_int_equal(rc, 0);

	char buff[100];
	size_t content_size = 100;

	rc = efs_readlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &link_inode,
				buff, &content_size);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(content_size, strlen(symlink_content));

	rc = memcmp(buff, symlink_content, strlen(symlink_content));

	ut_assert_int_equal(rc, 0);
}

/**
 * Setup for hardlink test
 * Description: Create a file
 * Strategy:
 *  1. Create file
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. File creation should be successful
 */
static int hardlink_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->file_inode = 0LL;
	ut_efs_obj->file_name = "test_hardlink_file";

	rc =  ut_file_create(state);
	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for hardlink test
 * Description: Create a file
 * Strategy:
 *  1. Delete file if not deleted
 *  2. Delete link if not deleted.
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. File and link deletion should be successful if exists
 */
static int hardlink_teardown(void **state)
{
	int rc = 0;

	struct ut_link_env *ut_link_obj = LINK_ENV_FROM_STATE(state);

	ut_link_obj->ut_efs_obj.file_inode = 0LL;

	if(ut_link_obj->ut_efs_obj.file_name != NULL) {

		rc =  ut_file_delete(state);
		ut_assert_int_equal(rc, 0);
	}

	if(ut_link_obj->link_name != NULL) {

		ut_link_obj->ut_efs_obj.file_name = ut_link_obj->link_name;

	 	rc =  ut_file_delete(state);
		ut_assert_int_equal(rc, 0);
	}

	return rc;
}

/**
 * Test for hardlink creation
 * Description: Create a hardlink for file
 * Strategy:
 *  1. Create hardlink for file
 *  2. Lookup for hardlink
 *  3. Verify output
 *  4. Get attributes and verify st_nlink count
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. Lookup for hardlink should be successful
 */
static void create_hardlink(void **state)
{
	int rc = 0;
	char *link_name = "test_hardlink";
	efs_ino_t file_inode = 0LL;

	struct ut_link_env *ut_link_obj = LINK_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_link_obj->ut_efs_obj;

	ut_link_obj->link_name = link_name;

	struct stat stat_out;
	memset(&stat_out, 0, sizeof(stat_out));

	time_t cur_time;
	time(&cur_time);

	rc = efs_link(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &ut_efs_obj->current_inode,
			link_name);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, link_name,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj->file_inode, file_inode);

	rc = efs_getattr(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_nlink, 2);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	} 
}

/**
 * Test for longname hardlink creation
 * Description: Create a hardlink with longname for file
 * Strategy:
 *  1. Create longname hardlink for file
 *  2. Lookup for hardlink
 *  3. Verify output
 *  4. Get attributes and verify st_nlink count and changed c_time
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. Lookup for hardlink should be successful
 */
static void create_longname255_hardlink(void **state)
{
	int rc = 0;
	efs_ino_t file_inode = 0LL;

	char *long_name = "123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901"
			"123456789012345678901234567890123456789012345678901";

	struct ut_link_env *ut_link_obj = LINK_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_link_obj->ut_efs_obj;

	ut_link_obj->link_name = long_name;

	struct stat stat_out;
	memset(&stat_out, 0, sizeof(stat_out));

	time_t cur_time;
	time(&cur_time);

	ut_assert_int_equal(255, strlen(long_name));

	rc = efs_link(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &ut_efs_obj->current_inode,
			long_name);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, long_name,
			&file_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj->file_inode, file_inode);

	rc = efs_getattr(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_nlink, 2);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	} 
}

/**
 * Test for hardlink creation and deletion of original file
 * Description: Create a hardlink for file and delete file
 * Strategy:
 *  1. Create hardlink for file
 *  2. Lookup for hardlink
 *  3. Delete created file
 *  4. Lookup for hardlink
 *  5. Verify output
 *  6. Get attributes and verify st_nlink count and changed c_time
 * Expcted behavior:
 *  1. No errors from EFS API.
 *  2. Lookup for hardlink should be successful
 */
static void create_hardlink_delete_original(void **state)
{
	int rc = 0;
	char *link_name = "test_hardlink";

	struct ut_link_env *ut_link_obj = LINK_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_link_obj->ut_efs_obj;

	ut_link_obj->link_name = link_name;

	efs_ino_t link_inode = 0LL;
	time_t cur_time;

	rc = efs_link(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &ut_efs_obj->current_inode,
			link_name);

	ut_assert_int_equal(rc, 0);

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, link_name, &link_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj->file_inode, link_inode);

	rc = efs_unlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, NULL, ut_efs_obj->file_name);

	ut_assert_int_equal(rc, 0);

	ut_efs_obj->file_name = NULL;
	
	time(&cur_time);

	struct stat stat_out;
	memset(&stat_out, 0, sizeof(stat_out));

	rc = efs_getattr(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
				&ut_efs_obj->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_nlink, 1);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
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
static void create_hardlink_delete_link(void **state)
{
	int rc = 0;
	char *link_name = "test_hardlink";

	struct ut_link_env *ut_link_obj = LINK_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_link_obj->ut_efs_obj;

	ut_link_obj->link_name = link_name;

	time_t cur_time;

	efs_ino_t file_inode = 0LL;

	rc = efs_link(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &ut_efs_obj->current_inode,
			link_name);
	ut_assert_int_equal(rc, 0);


	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, link_name, &file_inode);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(ut_efs_obj->file_inode, file_inode);

	time(&cur_time);

	rc = efs_unlink(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->current_inode, NULL, link_name);

	ut_assert_int_equal(rc, 0);

	ut_link_obj->link_name = NULL;

	struct stat stat_out;
	memset(&stat_out, '0', sizeof(stat_out));

	rc = efs_getattr(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
				&ut_efs_obj->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

/**
 * Setup for link_ops test group
 */
static int link_ops_setup(void **state)
{
	int rc = 0;
	struct ut_link_env *ut_link_obj = NULL;

	ut_link_obj = calloc(sizeof(struct ut_link_env), 1);

	ut_assert_not_null(ut_link_obj);

	*state = ut_link_obj;

	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for link_ops test group
 */
static int link_ops_teardown(void **state)
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
	char *test_log = "/var/log/cortx/test/ut/ut_efs.log";

	printf("Link Tests\n");

	rc = ut_load_config(CONF_FILE);
	if (rc != 0) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_log = ut_get_config("efs", "log_path", test_log);

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		goto out;
	}

	struct test_case test_list[] = {
		ut_test_case(create_symlink, NULL, symlink_teardown),
		ut_test_case(create_longname255_symlink, NULL, symlink_teardown),
		ut_test_case(create_hardlink, hardlink_setup,
				hardlink_teardown),
		ut_test_case(create_longname255_hardlink, hardlink_setup,
				hardlink_teardown),
		ut_test_case(create_hardlink_delete_original, hardlink_setup,
				hardlink_teardown),
		ut_test_case(create_hardlink_delete_link, hardlink_setup,
				hardlink_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, link_ops_setup,
				link_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
