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
#include <efs.h>
#include <debug.h>

static struct collection_item *cfg_items;

static int efs_initialized;

int efs_init(const char *config_path)
{
	struct collection_item *errors = NULL;
	int rc = 0;
	struct collection_item *item = NULL;
	char *log_path = NULL;
	char *log_level = NULL;
	efs_ctx_t ctx = EFS_NULL_FS_CTX;
	struct kvstore *kvstore = kvstore_get();

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

	RC_WRAP(get_config_item, "log", "path", cfg_items, &item);

	/** Path not specified, use default */
	if (item == NULL) {
		log_path = "/var/log/eos/efs/efs.log";
	} else {
		log_path = get_string_config_value(item, NULL);
	}
	
	//TODO add more for Utils_init().
	rc = utils_init();
	if (rc != 0) {
                rc = -EINVAL;
                goto err;
        }

	//TODO add more for nsal_init().
	rc = nsal_init();
        if (rc) {
                log_err("nsal_init failed");
                goto err;
        }
	
	//TODO add more for dsal_init().
	rc = dsal_init(cfg_items, 0);
	if (rc) {
		log_err("dsal_init failed");
		goto err;
	}
	item = NULL;
	
	//TODO efs_fs_init.
	rc =  efs_fs_init();
	if (rc) {
		log_err("efs_fs_init failed");
		goto err;
	}

err:
	if (rc) {
		free_ini_config_errors(errors);
		return rc;
	}

	log_debug("rc=%d, fs_ctx=%p", rc, ctx);

	/** @todo : remove all existing opened FD (crash recovery) */
	return rc;
}

int efs_fini(void)
{
	struct kvstore *kvstor = kvstore_get();

	assert(kvstor != NULL);
	efs_fs_fini();
	// TODO dsal_fini.	
	dsal_fini();
	nsal_fini();
	RC_WRAP(kvstor->kvstore_ops->fini);
	free_ini_config_errors(cfg_items);
	//TODO Utils_fini	
	utils_fini();
	
	return 0;
}

