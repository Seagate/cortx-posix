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

static void test_efs_fs_init()
{
        int rc = 0;
        rc = efs_fs_init(cfg_items);

	ut_assert_int_equal(rc, 0);
}

static void test_efs_cb(const struct efs_fs *fs, void *args)
{
	str256_t *fs_name = NULL;
	efs_fs_get_name(fs, &fs_name);

	printf("CB efs name = %s\n", fs_name->s_str);
}

static void test_efs_fs_scan()
{
	efs_fs_scan(test_efs_cb, NULL);
}

static void test_efs_fs_fini()
{
	int rc = 0;

	rc = efs_fs_fini();

	ut_assert_int_equal(rc, 0);
}

static int ut_efs_init()
{
	struct collection_item *errors = NULL;
	int rc = 0;
	struct kvstore *kvstore = kvstore_get();

	dassert(kvstore != NULL);
	rc = log_init("/var/log/eos/efs/efs.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		printf("Log init failed, rc: %d\n", rc);
		goto out;
	}

	rc = config_from_file("libkvsns", DEFAULT_CONFIG, &cfg_items,
				INI_STOP_ON_ERROR, &errors);
	if (rc) {
		log_debug("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = kvs_init(kvstore, cfg_items);
	if (rc) {
		log_debug("Failed to do kvstore init rc = %d", rc);
		goto out;
	}
out:
	if (rc) {
		free_ini_config_errors(errors);
		return rc;
	}

	log_debug("rc=%d efs_start done", rc);
	return rc;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char *test_log = "/var/log/eos/ut/efs/efs_fs.logs";

	if (argc > 1) {
		test_log = argv[1];
	}

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = ut_efs_init();
	if (rc) {
		printf("Failed to intitialize efs\n");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_efs_fs_init),
		ut_test_case(test_efs_fs_create),
		ut_test_case(test_efs_fs_delete),
		ut_test_case(test_efs_fs_scan),
		ut_test_case(test_efs_fs_fini),

	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	ut_fini();

	printf("Total tests  = %d\n", test_count);
	printf("Tests passed = %d\n", test_count-test_failed);
	printf("Tests failed = %d\n", test_failed);

	return 0;
}
