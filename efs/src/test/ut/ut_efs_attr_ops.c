/*
 * Filename: ut_efs_attr_ops.c
 * Description: Implementation tests for file attributes
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

/**
 * Test to set creation time
 * Description: Set creation time for file.
 * Strategy:
 *  1. Set ctime
 *  2. Verify setting ctime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. ctime recieved from getattr should match set ctime.
 */
static void set_ctime(void **state)
{
	struct ut_efs_params *ut_efs_objs = ENV_FROM_STATE(state);

	int rc = 0;
	int flag = STAT_CTIME_SET;

	time_t new_ctime;
	time(&new_ctime);

	struct tm *now_tm;
	now_tm = localtime(&new_ctime);

	// Increment current time by random value(1 hour in this case)
	now_tm->tm_hour += 1;

	new_ctime = mktime(now_tm);

	struct stat stat_in, stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_ctim.tv_sec = new_ctime;

	rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_in, flag);

	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
			&ut_efs_objs->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(0, difftime(new_ctime, stat_out.st_ctime));
}

/**
 * Test to set modification time
 * Description: Set modification time for file.
 * Strategy:
 *  1. Set mtime
 *  2. Verify setting mtime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. mtime recieved from getattr should match set mtime.
 */
static void set_mtime(void **state)
{
	struct ut_efs_params *ut_efs_objs = ENV_FROM_STATE(state);

	int rc = 0;
	int flag = STAT_MTIME_SET;

	time_t new_mtime, cur_time;
	time(&new_mtime);

	struct tm *now_tm;
	now_tm = localtime(&new_mtime);

	// Increment mtime by random value(1 hour in this case)
	now_tm->tm_hour += 1;

	new_mtime = mktime(now_tm);

	struct stat stat_in,stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_mtim.tv_sec = new_mtime;

	time(&cur_time);

	rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_in, flag);

	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(0, difftime(new_mtime, stat_out.st_mtime));

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

/**
 * Test to set access time
 * Description: Set access time for file.
 * Strategy:
 *  1. Set atime
 *  2. Verify setting atime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. atime recieved from getattr should match set atime.
 */
static void set_atime(void **state)
{
	struct ut_efs_params *ut_efs_objs = ENV_FROM_STATE(state);

	int rc = 0;
	int flag = STAT_ATIME_SET;

	time_t new_atime, cur_time;
	time(&new_atime);

	struct tm *now_tm;
	now_tm = localtime(&new_atime);

	// Increment atime by random value(1 hour in this case)
	now_tm->tm_hour += 1;

	new_atime = mktime(now_tm);

	struct stat stat_in, stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_atim.tv_sec = new_atime;

	time(&cur_time);

	rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_in, flag);
	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(0, difftime(new_atime, stat_out.st_atime));

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

/**
 * Test to set gid
 * Description: Set gid for file.
 * Strategy:
 *  1. Set gid
 *  2. Verify setting gid by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. gid recieved from getattr should match set gid.
 */
static void set_gid(void **state)
{
	struct ut_efs_params *ut_efs_objs = ENV_FROM_STATE(state);

	int rc = 0;
	int flag = STAT_GID_SET;

	gid_t new_gid =100;

	time_t cur_time;

	struct stat stat_in, stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_gid = new_gid;

	time(&cur_time);

	rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_in, flag);

	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_gid, new_gid);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

/**
 * Test to set uid
 * Description: Set uid for file.
 * Strategy:
 *  1. Set uid
 *  2. Verify setting uid by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. uid recieved from getattr should match set uid.
 */
static void set_uid(void **state)
{
	struct ut_efs_params *ut_efs_objs = ENV_FROM_STATE(state);

	int rc = 0;
	int flag = STAT_UID_SET;

	uid_t new_uid = 100;

	time_t cur_time;

	struct stat stat_in, stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_uid = new_uid;

	time(&cur_time);

	rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_in, flag);
	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				&ut_efs_objs->file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_uid, new_uid);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

/**
 * Setup for attr test group
 */
static int attr_test_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_objs = NULL;

	ut_efs_objs = calloc(sizeof(struct ut_efs_params), 1);
	ut_assert_not_null(ut_efs_objs);

	*state = ut_efs_objs;
	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	ut_efs_objs->file_name = "test_attr_file";
	ut_efs_objs->file_inode = 0LL;

	*state = ut_efs_objs;
	rc = ut_file_create(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for attr_ops test group
 */
static int attr_test_teardown(void **state)
{
	int rc = 0;

	rc = ut_file_delete(state);
	ut_assert_int_equal(rc, 0);

	rc = ut_efs_fs_teardown(state);
	ut_assert_int_equal(rc, 0);

	free(*state);

	return rc;
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/ut_efs.log";

	printf("Attribute Tests\n");

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
		ut_test_case(set_ctime, NULL, NULL),
		ut_test_case(set_mtime, NULL, NULL),
		ut_test_case(set_atime, NULL, NULL),
		ut_test_case(set_gid, NULL, NULL),
		ut_test_case(set_uid, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, attr_test_setup,
				attr_test_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
