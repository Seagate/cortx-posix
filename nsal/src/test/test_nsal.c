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

#include "test_nsal.h"

int ut_nsal_init()
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

	log_debug("rc=%d nsal_start done", rc);
	return rc;
}

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

	rc = ut_nsal_init();
	if (rc) {
		printf("Failed to intitialize nsal\n");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_init_ns),
		ut_test_case(test_scan_ns),
		ut_test_case(test_create_ns),
		ut_test_case(test_scan_ns),
		ut_test_case(test_delete_ns),
		ut_test_case(test_fini_ns),
	};

	int test_count = 6;
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	ut_fini();

	printf("Tests failed = %d", test_failed);

	return 0;
}
