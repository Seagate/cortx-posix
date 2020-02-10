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
	struct kvstore *kvstor = kvstore_get();
	char *kvstore_type = NULL;
	efs_ctx_t ctx = EFS_NULL_FS_CTX;

	dassert(kvstor);

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

	RC_WRAP(get_config_item, "kvstore", "type", cfg_items, &item);
	if (item == NULL) {
		log_err("KVStore type not specified\n");
		rc = -EINVAL;
		goto err;
	}

	kvstore_type = get_string_config_value(item, NULL);

	if (strcmp(kvstore_type, "mero") == 0) {
		rc = kvstore_init(kvstor, "mero", cfg_items, 0,
				  &eos_kvs_ops, &eos_kvs_index_ops,
				  &eos_kvs_kv_ops);

	} else if (strcmp(kvstore_type, "redis") == 0) {
#if 0 // @todo Need to port redis
		rc = kvstore_init(kvstor, "redis", cfg_items, 0,
				  redis_ops, redis_index_ops,
				  redis_kv_ops);
#endif
	} else {
		log_err("Invalid kvstore type %s", kvstore_type);
		rc = -EINVAL;
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

