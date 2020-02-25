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

#include "test_ns.h"

void test_ns_init()
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
		printf("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = kvs_init(kvstore, cfg_items);
	if (rc) {
		printf("Failed to do kvstore init rc = %d", rc);
		goto out;
	}

	rc = ns_init(cfg_items);

out:
	if (rc) {
		free_ini_config_errors(errors);
	}

	ut_assert_int_equal(rc, 0);
}

void test_ns_fini()
{
	int rc = 0;

	rc = ns_fini();

	ut_assert_int_equal(rc, 0);
}

void test_ns_create()
{
	int rc = 0;
	str256_t ns_name;
	char *name = "eosfs";
	struct namespace *ns;
	size_t ns_size = 0;

	str256_from_cstr(ns_name, name, strlen(name));
	rc = ns_create(&ns_name, &ns, &ns_size);

	ut_assert_int_equal(rc, 0);
}

void test_ns_delete()
{
	int rc = 0;
	str256_t ns_name;
	char *name = "eosfs1";
	struct namespace *fsn;
	struct namespace *ns;
	size_t ns_size = 0;

	str256_from_cstr(ns_name, name, strlen(name));
	rc = ns_create(&ns_name, &ns, &ns_size);

	fsn = malloc(ns_size);
	memcpy(fsn, ns, ns_size);

	rc = ns_delete(fsn);

	ut_assert_int_equal(rc, 0);

}

void test_cb(struct namespace *ns, size_t ns_size)
{
	str256_t *ns_name = NULL;
	ns_get_name(ns, &ns_name);
	printf("CB ns_name = %s\n", ns_name->s_str);
}

void test_ns_scan()
{
	int rc = 0;
	rc = ns_scan(test_cb);
	ut_assert_int_equal(rc,0);
}
