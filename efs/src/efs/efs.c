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
	item = NULL;

	RC_WRAP(get_config_item, "log", "level", cfg_items, &item);
	if (item == NULL) {
		log_level = "LEVEL_INFO";
	} else {
		log_level = get_string_config_value(item, NULL);
	}
	rc = log_init(log_path, log_level_no(log_level));
	if (rc != 0) {
		rc = -EINVAL;
		goto err;
	}

	rc = dstore_init(cfg_items, 0);
	if (rc) {
		log_err("dstore_init failed");
		goto err;
	}
	item = NULL;

	rc = kvs_init(cfg_items, 0);
	if (rc) {
		log_err("kvs_init failed");
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

	RC_WRAP(kvstor->kvstore_ops->fini);
	free_ini_config_errors(cfg_items);
	return 0;
}

