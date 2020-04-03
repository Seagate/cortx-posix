/*
 * Filename: efs_attr_ops.c
 * Description: Implementation tests for file attributes
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
 * Test to set creation time
 * Description: Set creation time for file.
 * Strategy:
 *  1. Set ctime
 *  2. Verify setting ctime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. ctime recieved from getattr should match set ctime.
 */
static void set_ctime(void)
{
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

	rc = efs_setattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_in, flag);

	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.file_inode, &stat_out);

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
static void set_mtime(void)
{
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

	rc = efs_setattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_in, flag);

	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_out);

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
static void set_atime(void)
{
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

	rc = efs_setattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_in, flag);
	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_out);

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
static void set_gid(void)
{
	int rc = 0;
	int flag = STAT_GID_SET;

	gid_t new_gid =100;

	time_t cur_time;

	struct stat stat_in, stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_gid = new_gid;

	time(&cur_time);

	rc = efs_setattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_in, flag);

	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_out);

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
static void set_uid(void)
{
	int rc = 0;
	int flag = STAT_UID_SET;

	uid_t new_uid = 100;

	time_t cur_time;

	struct stat stat_in, stat_out;
	memset(&stat_in, 0, sizeof(stat_in));
	memset(&stat_out, 0, sizeof(stat_out));

	stat_in.st_uid = new_uid;

	time(&cur_time);

	rc = efs_setattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_in, flag);
	ut_assert_int_equal(rc, 0);

	rc = efs_getattr(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
				&ut_efs_obj.file_inode, &stat_out);

	ut_assert_int_equal(rc, 0);

	ut_assert_int_equal(stat_out.st_uid, new_uid);

	if(difftime(stat_out.st_ctime, cur_time) < 0) {
		ut_assert_true(0);
	}
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/attr_ops.log";

	printf("Attribute Tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = ut_efs_fs_setup();
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	ut_efs_obj.file_name = "test_attr_file";
	ut_efs_obj.file_inode = 0LL;

	rc = efs_creat(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, ut_efs_obj.file_name, 0755,
			&ut_efs_obj.file_inode);
	if (rc) {
		printf("Failed to create file %s\n", ut_efs_obj.file_name);
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(set_ctime),
		ut_test_case(set_mtime),
		ut_test_case(set_atime),
		ut_test_case(set_gid),
		ut_test_case(set_uid),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	rc = efs_unlink(ut_efs_obj.efs_fs, &ut_efs_obj.cred,
			&ut_efs_obj.current_inode, NULL, ut_efs_obj.file_name);

	if (rc) {
		printf("Failed to delete file %s\n", ut_efs_obj.file_name);
	}

	ut_efs_fs_teardown();

	ut_fini();

	printf("Total tests  = %d\n", test_count);
	printf("Tests passed = %d\n", test_count-test_failed);
	printf("Tests failed = %d\n", test_failed);

	return 0;
}
