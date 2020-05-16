/*****************************************************************************/
/*
 * Filename:		dsal_test_lib.c
 * Description:		Code shared across the dsal_test_* modules.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 */

/******************************************************************************/
#include "dsal_test_lib.h"
#include <stdio.h> /* *printf */
#include <assert.h> /* assert */
#include <stdlib.h> /* abort() */
#include <ini_config.h> /* ini file parser */
#include "dstore.h" /* dstore init/fini */
#include "common/log.h" /* logger init */

/* Path to EFS config for DSAL tests */
#define EFS_TEST_CONF_PATH "/etc/efs/efs.conf"

/* Default log level for DSAL tests */
#define DEFAULT_LOG_LEVEL "LEVEL_DEBUG"

/******************************************************************************/
struct dstore *dstore;
dstore_oid_t def_object = {0};

/******************************************************************************/
/* Module-wide "setup" action.
 * Sets up the environment for all testcases in this module.
 */
void dtlib_setup(int argc, char *argv[])
{
	int rc = 0;
	const char* config_path = EFS_TEST_CONF_PATH;
	struct collection_item *cfg_items = NULL;
	struct collection_item *errors = NULL;
	struct collection_item *item = NULL;
	char *log_path = NULL;
	char *log_level = NULL;

	/* Enable log redirection if the path has been provided.
	 * Otherwise, use stderr/stdout.
	 */
	if (argc == 2) {
		rc = ut_init(argv[1]);
		if (rc) {
			fprintf(stderr, "ut_init() failed\n");
			goto die;
		}
	}

	rc = config_from_file("libkvsns", config_path, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		fprintf(stderr, "Failed to load ini file.\n");
		goto die;
	}

	(void) get_config_item("log", "path", cfg_items, &item);
	if (item != NULL) {
		log_path = get_string_config_value(item, NULL);
		item = NULL;
	} else {
		fprintf(stderr, "Log path is not specified.\n");
		goto die;
	}


	(void) get_config_item("log", "level", cfg_items, &item);
	if (item != NULL) {
		log_level = get_string_config_value(item, NULL);
		item = NULL;
	} else {
		log_level = DEFAULT_LOG_LEVEL;
	}

	rc = log_init(log_path, log_level_no(log_level));
	if (rc) {
		fprintf(stderr, "Failed to initialize logger.\n");
		goto die;
	}

	rc = dstore_init(cfg_items, 0);
	if (rc) {
		fprintf(stderr, "Failed to initialize dstore.\n");
		goto die;
	}

	dstore = dstore_get();

	rc = dstore_get_new_objid(dstore, &def_object);
	if (rc) {
		fprintf(stderr, "Cannot generete UFID for the default object\n");
		goto die;
	}

	return;

die:
	fprintf(stderr, "Failed to initialize test group environment %d\n", rc);
	abort();
}

/******************************************************************************/
/* Module-wide "teardown" action.
 * Cleans up the environment for all testcases in this module.
 */
void dtlib_teardown(void)
{
	/* TODO: no reliable fini() so far in DSAL. */
}

/*****************************************************************************/
struct dstore *dtlib_dstore(void)
{
	return dstore;
}

/*****************************************************************************/
const dstore_oid_t *dtlib_def_obj(void)
{
	return &def_object;
}

/*****************************************************************************/
