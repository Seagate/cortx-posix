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

void test_init_ns()
{
	int rc = 0;

	rc = ns_init(cfg_items);

	ut_assert_int_equal(rc, 0);
}

void test_fini_ns()
{
	int rc = 0;

	rc = ns_fini();

	ut_assert_int_equal(rc, 0);
}

void test_create_ns()
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

void test_delete_ns()
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

void test_scan_ns()
{
	int rc = 0;
	rc = ns_scan(test_cb);
	ut_assert_int_equal(rc,0);
}
