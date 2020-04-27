/*
 * Filename:         efs.c
 * Description:      EOS file system interfaces

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system interfaces.
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <ini_config.h>
#include <common/log.h>
#include <common/helpers.h>
#include <eos/eos_kvstore.h>
#include <dstore.h>
#include <dsal.h> /* dsal_init,fini */
#include <efs.h>
#include <fs.h>
#include <debug.h>
#include <management.h>
#include <nsal.h> /* nsal_init,fini */
#include <ecfg.h> /* cfg_init,fini */

static int efs_initialized;

struct efs_module_state {
	struct ecfg *config;
	/* Add efs specific tunable attributes here */
};

static struct efs_module_state *state = NULL;

/* create the efs state */
int efs_module_state_init(struct efs_module_state **out)
{
	int rc = 0;
	struct efs_module_state *st_local = NULL;

	st_local = malloc(sizeof(struct efs_module_state));
	if (st_local == NULL) {
		rc = -ENOMEM;
		goto out;
	}
	st_local->config = NULL;

out:
	*out = st_local;
	return rc;
}

/* finalize the efs state */
void efs_module_state_fini(struct efs_module_state *efs_state)
{
	free(efs_state);
}

int efs_init(const char *config_path)
{
	int rc = 0;
	char *log_path = NULL;
	char *log_level = NULL;
	struct ecfg *ecfg_local;
	struct collection_item *cfg_items = NULL;

	/** only initialize efs once */
	if (__sync_fetch_and_add(&efs_initialized, 1)) {
		return 0;
	}

	rc = efs_module_state_init(&state);
	if (rc != 0) {
		goto out;
	}
	rc = cfg_init(config_path, &ecfg_local);
	if (ecfg_local == NULL) {
		goto out;
	}
	state->config = ecfg_local;

	/* Fetch log-path & log-level */
	rc = cfg_get(state->config, "log", "path", &log_path);
	if (rc != 0) {
		/* default log-path */
		log_path = "/var/log/eos/efs/efs.log";
	}
	rc = cfg_get(state->config, "log", "level", &log_level);
	if (rc != 0){
		/* default log-level */
		log_level = "LEVEL_INFO";
	}
	rc = log_init(log_path, log_level_no(log_level));
        if (rc != 0) {
                rc = -EINVAL;
                goto out;
        }
	/* Fetch collection-items */
	rc = cfg_get_collection(state->config, &cfg_items);
	if (rc != 0) {
		log_err("ecfg_get_collection failed, rc=%d", rc);
		goto log_cleanup;
	}
	rc = utils_init(cfg_items);
	if (rc != 0) {
		log_err("utils_init failed, rc=%d", rc);
                goto log_cleanup;
        }
	rc = nsal_init(cfg_items);
        if (rc) {
                log_err("nsal_init failed, rc=%d", rc);
                goto utils_cleanup;
        }
	rc = dsal_init(cfg_items, 0);
	if (rc) {
		log_err("dsal_init failed, rc=%d", rc);
		goto nsal_cleanup;
	}
	rc = efs_fs_init(cfg_items);
	if (rc) {
		log_err("efs_fs_init failed, rc=%d", rc);
		goto dsal_cleanup;
	}
	rc = management_init();
	if (rc) {
		log_err("management_init failed, rc=%d", rc);
                goto efs_fs_cleanup;
        }
	goto out;
efs_fs_cleanup:
	efs_fs_fini();
dsal_cleanup:
	dsal_fini();
nsal_cleanup:
	nsal_fini();
utils_cleanup:
	utils_fini();
log_cleanup:
	log_fini();
out:
	if (rc) {
		cfg_fini(state->config);
		efs_module_state_fini(state);
		return rc;
	}

	/** @todo : remove all existing opened FD (crash recovery) */
	return rc;
}

int efs_fini(void)
{
	int rc = 0;
	//TODO management_fini.
	rc = efs_fs_fini();
	if (rc) {
                log_err("efs_fs_fini failed, rc=%d", rc);
        }
	rc = dsal_fini();
	if (rc) {
                log_err("dsal_fini failed, rc=%d", rc);
        }
	rc = nsal_fini();
	if (rc) {
                log_err("nsal_fini failed, rc=%d", rc);
        }
	rc = utils_fini();
	if (rc) {
		log_err("utils_fini failed, rc=%d", rc);
        }
	rc = log_fini();
	if (rc) {
                log_err("log_fini failed, rc=%d ", rc);
        }
	cfg_fini(state->config);
	efs_module_state_fini(state);
	return rc;
}
