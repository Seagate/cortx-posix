/*1
 * Filename: test_tenant.c
 * Description: Implementation tests for tenant.
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
#include "tenant.h"
#include "namespace.h"
#include "nsal.h"
#include "ut.h"

#define EFS_DEFAULT_CONFIG 				"/etc/cortx/cortxfs.conf"

struct collection_item *cfg_items;

static void test_tenant_module_init(void)
{
	struct collection_item *errors = NULL;
	int rc = 0;

	rc = log_init("/var/log/cortx/fs/efs.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		printf("Log init failed, rc: %d\n", rc);
		goto out;
	}

	rc = config_from_file("libkvsns", EFS_DEFAULT_CONFIG, &cfg_items,
							INI_STOP_ON_ERROR, &errors);
	if (rc) {
		printf("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = nsal_module_init(cfg_items);

out:
	if (rc) {
		free_ini_config_errors(errors);
	}

	ut_assert_int_equal(rc, 0);
}

static void test_tenant_module_fini(void)
{
	int rc = 0;

	rc = nsal_module_fini();
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to create new (non-existing) tenant entry.
 * Description: Create new (non-existing) tenant entry and store information
 * in kvs.
 * Strategy:
 * 1. Create tenant entry by name.
 * 2. dump information in kvs.
 * Expected behavior:
 * 1. No errors from tenant API.
 */
static void test_tenant_create(void)
{
	int rc = 0;
	str256_t tenant_name;
	char *name = "tenant";
	struct tenant *tenant;
	const char *dummy = "option";

	str256_from_cstr(tenant_name, name, strlen(name));
	rc = tenant_create(&tenant_name, &tenant, 1, dummy);
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to delete (existing) tenant entry.
 * Description: Delete (existing) tenant entry and purge information from kvs.
 * Strategy:
 * 1. Create a tenant entry by name.
 * 2. Delete tenant entry created in step 1.
 * Expected behavior:
 * 1. No errors from tenant API.
 */
static void test_tenant_delete(void)
{
	int rc = 0;
	str256_t tenant_name;
	char *name = "tenant1";
	struct tenant *tenant;
	const char *dummy = "option";

	str256_from_cstr(tenant_name, name, strlen(name));
	rc = tenant_create(&tenant_name, &tenant, 1, dummy);
	ut_assert_int_equal(rc, 0);

	rc = tenant_delete(tenant);
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to delete (non-existing) tenant entry.
 * Description: Delete (non-existing) tenant entry kvs.
 * Strategy:
 * 1. Create a tenant entry by name.
 * 2. Delete tenant entry created in step 1.
 * 3. Now,  delete tenant entry created in step 1.
 * Expected behavior:
 * 1. error ENOENT from tenant API.
 */
static void test_tenant_nonexist_delete(void)
{
	int rc = 0;
	str256_t tenant_name;
	char *name = "tenant1";
	struct tenant *tenant;
	const char *dummy = "option";

	str256_from_cstr(tenant_name, name, strlen(name));
	rc = tenant_create(&tenant_name, &tenant, 1, dummy);
	ut_assert_int_equal(rc, 0);

	rc = tenant_delete(tenant);
	ut_assert_int_equal(rc, 0);
	rc = tenant_delete(tenant);
	ut_assert_int_not_equal(rc, ENOENT);
}

/* helper function for test_tenant_scan */
static int test_cb(void *cb_ctx, struct tenant *tenant)
{
	int rc = 0;
	str256_t *tenant_name = NULL;

	tenant_get_name(tenant, &tenant_name);
	if (tenant_name == NULL) {
		rc = -ENOENT;
		goto out;
	}

	printf("CB tenant_name = %s\n", tenant_name->s_str);
out:
	return rc;
}

/**
 * Test to list all tenant entry.
 * Description: list all entry and corresponding information.
 * strored in meta_index.
 * Strategy:
 * 1. Scan each tenant entry in meta_index.
 * Expected behavior:
 * 1. List tenant entry and corresponding infomation.
 */
static void  test_tenant_scan(void)
{
	int rc = 0;
	void *cb_ctx = (void *)0;
	rc = tenant_scan(test_cb, cb_ctx);
	ut_assert_null(cb_ctx);
	ut_assert_int_equal(rc, 0);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int test_count = 0;
	int test_failed = 0;
	char *test_logs = "/var/log/eos/test/ut/nsal/tenant.logs";

	if (argc > 1) {
		test_logs = argv[1];
	}

	rc = ut_init(test_logs);
	if (rc != 0) {
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_tenant_module_init, NULL, NULL),
		ut_test_case(test_tenant_scan, NULL, NULL),
		ut_test_case(test_tenant_create, NULL, NULL),
		ut_test_case(test_tenant_scan, NULL, NULL),
		ut_test_case(test_tenant_delete, NULL, NULL),
		ut_test_case(test_tenant_nonexist_delete, NULL, NULL),
		ut_test_case(test_tenant_module_fini, NULL, NULL),
	};

	test_count = sizeof(test_list) / sizeof(test_list[0]);
	test_failed = ut_run(test_list, test_count, NULL, NULL);
	ut_fini();
	ut_summary(test_count, test_failed);
	return 0;
}
