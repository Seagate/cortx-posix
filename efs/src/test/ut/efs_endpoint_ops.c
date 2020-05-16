/*
  Filename: efs_endpoint.c
 * Description: implementation tests for efs_endpoint.
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

#include "efs_endpoint.h"
#include "nsal.h"
#include "internal/fs.h"

struct collection_item *cfg_items;

static void test_efs_endpoint_create()
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));

	rc = efs_fs_create(&fs_name);
	ut_assert_int_equal(rc, 0);

	const char *endpoint_options  = "{ \"proto\": \"nfs\", \"mode\": \"rw\", \"secType\": \"sys\", \"Filesystem_id\": \"192.168\", \"client\": \"1\", \"clients\": \"*\", \"Squash\": \"no_root_squash\", \"access_type\": \"RW\", \"protocols\": \"4\" }";

	rc = efs_endpoint_create(&fs_name, endpoint_options);
	ut_assert_int_equal(rc, 0);
}

static void test_efs_endpoint_delete()
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));

	rc = efs_endpoint_delete(&fs_name);
	ut_assert_int_equal(rc, 0);

	rc = efs_fs_delete(&fs_name);
	ut_assert_int_equal(rc, 0);
}

static int test_endpoint_cb(const struct efs_fs_list_entry *list, void *args)
{
	int rc = 0;

	if (list->endpoint_info == NULL) {
		printf("CB efs name %s not exported\n", list->fs_name->s_str);
		rc = -ENOENT;
		goto out;
	} else {
		printf("CB efs name %s exported with %s\n",
		       list->fs_name->s_str, (char *)list->endpoint_info);
	}
out:
	return rc;
}

static void test_efs_endpoint_scan()
{
	int rc = 0;
	rc = efs_fs_scan_list(test_endpoint_cb, (void*)0);
	ut_assert_int_equal(rc, 0);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/endpoint_ops.logs";

	if (argc > 1) {
		test_log = argv[1];
	}

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = efs_init(EFS_DEFAULT_CONFIG);
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_efs_endpoint_create, NULL, NULL),
		ut_test_case(test_efs_endpoint_scan, NULL, NULL),
		ut_test_case(test_efs_endpoint_delete, NULL, NULL),
		ut_test_case(test_efs_endpoint_scan, NULL, NULL),

	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
