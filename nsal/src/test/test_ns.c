/*1
 * Filename: test_ns.c
 * Description: Implementation tests for namespace.
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

#include <errno.h>
#include <string.h>
#include "debug.h"
#include "common/log.h"
#include "str.h"
#include "namespace.h"
#include "nsal.h"
#include "ut.h"

#define DEFAULT_CONFIG "/etc/efs/efs.conf"

static struct collection_item *cfg_items;

static void test_ns_module_init(void)
{
	struct collection_item *errors = NULL;
	int rc = 0;

	rc = log_init("/var/log/cortx/efs/efs.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		printf("Log init failed, rc: %d\n", rc);
		goto out;
	}

	rc = config_from_file("libkvsns", DEFAULT_CONFIG, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		printf("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = nsal_module_init(cfg_items);
	if (rc) {
		printf("nsal init failed rc = %d", rc);
		rc = -rc;
		goto out;
	}

out:
	if (rc) {
		free_ini_config_errors(errors);
	}

	ut_assert_int_equal(rc, 0);
}

static void test_ns_module_fini(void)
{
	int rc = 0;

	rc = nsal_module_fini();
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to create new (non-existing) namespace entry and delete.
 * Description: Create new (non-existing) namespace entry and then delete.
 * in kvs.
 * Strategy:
 * 1. Create namespace entry by name.
 * 2. dump information in kvs.
 * 3. Delete entry created by step 1.
 * Expected behavior:
 * 1. No errors from tenant API.
 */
static void test_ns_create_delete(void)
{
	int rc = 0;
	str256_t ns_name;
	char *name = "eosfs";
	struct namespace *ns;
	size_t ns_size = 0;

	str256_from_cstr(ns_name, name, strlen(name));
	rc = ns_create(&ns_name, &ns, &ns_size);
	ut_assert_int_equal(rc, 0);
	rc = ns_delete(ns);
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to delete (non-existing) namespace entry.
 * Description: Delete (non-existing) namespace entry kvs.
 * Strategy:
 * 1. Create a namespace entry by name.
 * 2. Delete namespace entry created by step 1.
 * 3. Now, delete tenant entry created by step 1.
 * Expected behavior:
 * 1. error ENOENT from tenant API.
 */
static void test_ns_nonexist_delete(void)
{
	int rc = 0;
	str256_t ns_name;
	char *name = "test";
	struct namespace *ns;
	size_t ns_size = 0;

	str256_from_cstr(ns_name, name, strlen(name));
	rc = ns_create(&ns_name, &ns, &ns_size);
	rc = ns_delete(ns);
	ut_assert_int_equal(rc, 0);
	rc = ns_delete(ns);
	ut_assert_int_equal(rc, -ENOENT);
}

/* helper function for test_ns_scan */
static void test_cb(struct namespace *ns, size_t ns_size)
{
	str256_t *ns_name = NULL;
	ns_get_name(ns, &ns_name);
	printf("CB ns_name = %s\n", ns_name->s_str);
}

/**
 * Test to list all namespace entry.
 * Description: list all namespace entries strored in meta_index.
 * Strategy:
 * 1. Scan each namespace entry in meta_index.
 * Expected behavior:
 * 1. List namespace entry and corresponding infomation.
 */
static void test_ns_scan(void)
{
	int rc = 0;

	rc = ns_scan(test_cb);
	ut_assert_int_equal(rc,0);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int test_count = 0;
	int test_failed = 0;
	char *test_logs = "/var/log/eos/test/ut/nsal/ns_ops.logs";

	if (argc > 1) {
		test_logs = argv[1];
	}

	rc = ut_init(test_logs);
	if (rc != 0) {
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_ns_module_init, NULL, NULL),
		ut_test_case(test_ns_scan, NULL, NULL),
		ut_test_case(test_ns_create_delete, NULL, NULL),
		ut_test_case(test_ns_nonexist_delete, NULL, NULL),
		ut_test_case(test_ns_module_fini, NULL, NULL),
	};

	test_count = sizeof(test_list) / sizeof(test_list[0]);
	test_failed = ut_run(test_list, test_count, NULL, NULL);
	ut_fini();
	ut_summary(test_count, test_failed);
	return 0;
}
