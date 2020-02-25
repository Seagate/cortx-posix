/*
 * Filename: test_nsal.c
 * Description: Executes nsal tests
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


int main(int argc, char *argv[])
{
	int rc = 0;
	char *test_logs = "/var/log/eos/test.logs";

	if (argc > 1) {
		test_logs = argv[1];
	}

	rc = ut_init(test_logs);
	if (rc != 0) {
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_ns_init),
		ut_test_case(test_ns_create),
		ut_test_case(test_ns_scan),
		ut_test_case(test_ns_delete),
		ut_test_case(test_ns_fini),
	};

	int test_count = 5;
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	ut_fini();

	printf("Tests failed = %d", test_failed);

	return 0;
}
