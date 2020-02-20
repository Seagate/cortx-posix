#include <stdio.h>
#include <stdlib.h>
#include <ini_config.h>
#include "eos/eos_kvstore.h"
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <common/log.h>
#include "str.h"
#include <namespace.h>

#define DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

static struct collection_item *cfg_items;
struct namespace *ns = NULL;
struct kvs_idx kv_index;

int nsal_start(const char *config_path)
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

	rc = config_from_file("libkvsns", config_path, &cfg_items,
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

int test_ns_init()
{
	int rc = 0;
	rc = ns_init(cfg_items);
	return rc;
}

int test_ns_fini()
{
	int rc = 0;
	rc = ns_fini();
	return rc;
}

int test_ns_create(char *name)
{
	int rc = 0;
	str256_t ns_name;
	printf("j\n");
	str256_from_cstr(ns_name, name, strlen(name));
	rc = ns_create(&ns_name, &ns);

	return rc;
}

int test_ns_delete(struct namespace *ns)
{
	int rc = 0;
	rc = ns_delete(ns);

	return rc;
}

int test_ns_scan()
{
	int rc = 0;
	struct ns_itr *iter = NULL;
	rc = ns_scan(&iter);
	while (rc == 0) {
		rc = ns_scan(&iter);
	}

	return rc;
}

int main(int argc, char *argv[])
{
	int rc = 0;

	printf("Namespace test\n");

	rc = nsal_start(DEFAULT_CONFIG);
	if (rc) {
		printf("Failed to do nsal_start rc = %d", rc);
	}

	rc = test_ns_init();
	if (rc) {
		printf("Failed namespace failed.\n");
	}

	rc = test_ns_create("test");
	if (rc != 0) {
		printf("Failed to create namespace, rc=%d\n", rc);
	}

	rc = test_ns_create("test1");
	if (rc != 0) {
		printf("Failed to create namespace, rc=%d\n", rc);
	}

	rc = test_ns_create("test2");
	if (rc != 0) {
		printf("Failed to create namespace, rc=%d\n", rc);
	}

	rc = test_ns_scan();
	if (rc == 0 ) {
		printf("passesd\n");
	}
/*	rc = test_ns_delete(ns);
	if (rc !=0 ) {
		printf("namespace detetion failed rc=%d\n", rc);
	} 

	rc = test_ns_delete(ns);
	if (rc !=0 ) {
		printf("namespace detetion failed expected rc=%d\n", rc);
	}

	rc = test_ns_fini();
	if (rc) {
		printf("Failed namespace finialize.\n");
	}
*/
	printf("All done.\n");

	return 0;
}

