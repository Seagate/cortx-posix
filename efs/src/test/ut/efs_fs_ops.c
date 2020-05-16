/*
  Filename: efs_fs.c
 * Description: mplementation tests for efs_fs
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 */

#include "efs_fs.h"
#include "nsal.h"

struct collection_item *cfg_items;

static void test_efs_fs_create()
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc = efs_fs_create(&fs_name);

	ut_assert_int_equal(rc, 0);
}

static void test_efs_fs_delete()
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc =  efs_fs_delete(&fs_name);

	ut_assert_int_equal(rc, 0);
}

static int test_efs_cb(const struct efs_fs_list_entry *list,  void *args)
{
	int rc = 0;

	if (args != NULL) {
		rc = -EINVAL;
		goto out;
	}

	printf("CB efs name = %s\n", list->fs_name->s_str);
out:
	return rc;
}

static void test_efs_fs_scan()
{
	int rc = 0;

	rc = efs_fs_scan_list(test_efs_cb, (void *)0);
	ut_assert_int_equal(rc, 0);
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/efs/efs_fs.logs";

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		exit(1);
	}
	rc = efs_init(EFS_DEFAULT_CONFIG);
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_efs_fs_create, NULL, NULL),
		ut_test_case(test_efs_fs_delete, NULL, NULL),
		ut_test_case(test_efs_fs_scan, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	efs_fini();

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
