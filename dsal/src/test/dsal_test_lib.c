/*
 * Filename:		dsal_test_lib.c
 * Description:		Code shared across the dsal_test_* modules.
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
 */

#include "dsal_test_lib.h"
#include <stdio.h> /* *printf */
#include <assert.h> /* assert */
#include <stdlib.h> /* abort() */
#include <ini_config.h> /* ini file parser */
#include "dstore.h" /* dstore init/fini */
#include "common/log.h" /* logger init */

/* Path to EFS config for DSAL tests */
#define EFS_TEST_CONF_PATH "/etc/cortx/cortxfs.conf"

/* Default log level for DSAL tests */
#define DEFAULT_LOG_LEVEL LEVEL_DEBUG

/******************************************************************************/
static struct dstore *dstore;
static dstore_oid_t def_object = {0};
static dstore_oid_t def_multi_obj[NUM_OF_OBJECTS] = {{0}};

/* Global variables for efs config for m0_filesystem_stats */
char *server = NULL, *client = NULL;
const char* config_path = EFS_TEST_CONF_PATH;
struct collection_item *cfg_items = NULL;
struct collection_item *errors = NULL;
struct collection_item *item = NULL;

/* Read efs config parameter needed for m0_filesystem_stats */
int dtlib_get_motr_config_params(void)
{
	int rc = 0, len;

	rc = config_from_file("libkvsns", config_path, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		fprintf(stderr, "Failed to load ini file.\n");
		goto die;
	}

	(void) get_config_item("motr", "local_addr", cfg_items, &item);
	if (item != NULL) {
		server = get_string_config_value(item, NULL);
		item = NULL;
	}
	else
		goto die;

	(void) get_config_item("motr", "ha_addr", cfg_items, &item);
	if (item != NULL) {
		client = get_string_config_value(item, NULL);
		item = NULL;

		/* for m0_filesystems_stats call we have to mention
		   different client address. Hence need to tweak the existing
		   adress by changing last digit.
		   Below logic will change last digit to 9 if it is less
		   than 9, else it sets it to 8 */
		len = strlen(client);
		if (client[len-1] < '9')
			client[len-1] = '9';
		else
			client[len-1] = '8';
	}
	else {
		free(server);
		goto die;
	}
	free_ini_config(cfg_items);
	return SUCCESS;

die:
	free_ini_config_errors(errors);
	free_ini_config(cfg_items);
	fprintf(stderr, "Failed to initialize test group environment %d\n", rc);
	return FAILURE;
}

/* Method to transfer the read values */
void dtlib_get_clnt_svr(char **srv, char **clnt)
{
	*srv = server;
	*clnt = client;
}

/******************************************************************************/
/* Module-wide "setup" action.
 * Sets up the environment for all testcases in this module.
 */
int dtlib_common_setup(int argc, char *argv[])
{
	int rc = 0;
	const char* config_path = EFS_TEST_CONF_PATH;
	struct collection_item *cfg_items = NULL;
	struct collection_item *errors = NULL;
	struct collection_item *item = NULL;
	char *log_path = NULL;
	char *log_level_str = NULL;
	log_level_t log_level;

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
		log_level_str = get_string_config_value(item, NULL);
		log_level = log_level_no(log_level_str);
		free(log_level_str);
		item = NULL;
	} else {
		log_level = DEFAULT_LOG_LEVEL;
	}

	rc = log_init(log_path, log_level);
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

	free(log_path);
	free_ini_config(cfg_items);
	return SUCCESS;

die:
	if (log_path) {
		free(log_path);
	}
	free_ini_config_errors(errors);
	free_ini_config(cfg_items);
	fprintf(stderr, "Failed to initialize test group environment %d\n", rc);
	return FAILURE;
}

/* Setup for single DSAL object operations */
int dtlib_setup(int argc, char *argv[])
{
	int rc = 0;

	dtlib_common_setup(argc, argv);
	rc = dstore_get_new_objid(dstore, &def_object);
	if (rc) {
		fprintf(stderr,
			"Cannot generete UFID for the default object\n");
		goto die;
	}

	return SUCCESS;

die:
	fprintf(stderr, "Failed to initialize test group environment %d\n", rc);
	return FAILURE;
}

/* setup for multiple DSAL objects */
int dtlib_setup_for_multi(int argc, char *argv[])
{
	int rc = 0, i;

	dtlib_get_motr_config_params();
	dtlib_common_setup(argc, argv);

	for (i = 0; i < NUM_OF_OBJECTS; i++) {
		rc = dstore_get_new_objid(dstore, &def_multi_obj[i]);
		if (rc) {
			fprintf(stderr,
			      "Cannot generete UFID for the default object\n");
			goto die;
		}
	}

	return SUCCESS;

die:
	fprintf(stderr, "Failed to initialize test group environment %d\n", rc);
	return FAILURE;
}

/******************************************************************************/
/* Module-wide "teardown" action.
 * Cleans up the environment for all testcases in this module.
 */
void dtlib_teardown(void)
{
	dstore_fini(dstore);
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
/* pass on multiple FIDs to the caller */
void dtlib_get_objids(dstore_oid_t oids[])
{
	int i;

	for (i = 0; i < NUM_OF_OBJECTS; i++) {
		oids[i] = def_multi_obj[i];
	}
}
