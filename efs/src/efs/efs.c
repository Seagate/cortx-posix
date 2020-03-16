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
#include <dsal.h>
#include <efs.h>
#include <fs.h>
#include <debug.h>
#include <management.h>
#include <nsal.h>

static struct collection_item *cfg_items;

static int efs_initialized;

int efs_init(const char *config_path)
{
	struct collection_item *errors = NULL;
	int rc = 0;
	
	/** only initialize efs once */
	if (__sync_fetch_and_add(&efs_initialized, 1)) {
		return 0;
	}

	rc = config_from_file("libkvsns", config_path, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		free_ini_config_errors(errors);
		rc = -rc;
		goto err;
	}

	rc = utils_init(cfg_items);
	if (rc != 0) {
		log_err("utils_init failed, rc=%d", rc);
                goto err;
        }
	rc = nsal_init(cfg_items);
        if (rc) {
                log_err("nsal_init failed, rc=%d", rc);
                goto err1;
        }
	rc = dsal_init(cfg_items, 0);
	if (rc) {
		log_err("dsal_init failed, rc=%d", rc);
		goto err2;
	}
	rc = efs_fs_init(cfg_items);
	if (rc) {
		log_err("efs_fs_init failed, rc=%d", rc);
		goto err3;
	}

	rc = management_init();
	if (rc) {
		log_err("management_init failed, rc=%d", rc);
                goto err4;
        }
	goto err;
err4:
	efs_fs_fini();
err3:
	dsal_fini();
err2:
	nsal_fini();
err1:
	utils_fini();
err:
	if (rc) {
		free_ini_config_errors(errors);
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
                goto err;
        }
	rc = dsal_fini();
	if (rc) {
                log_err("dsal_fini failed, rc=%d", rc);
                goto err;
        }
	rc = nsal_fini();
	if (rc) {
                log_err("nsal_fini failed, rc=%d", rc);
                goto err;
        }
	free_ini_config_errors(cfg_items);
	rc = utils_fini();
	if (rc) {
        	log_err("utils_fini failed, rc=%d", rc);
                goto err;
        }
err:
        return rc;
}
