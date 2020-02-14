#include <stdio.h>
#include <stdlib.h>
#include <ini_config.h>
#include "eos/eos_kvstore.h"
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <common/log.h>
#include "str.h"

#define DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

struct test_key {
	char type;
	str256_t name;
} test_key;

static struct collection_item *cfg_items;
struct kvs_idx kv_index;

int nsal_start(const char *config_path)
{
	struct collection_item *errors = NULL;
	int rc = 0;

	rc = log_init("/var/log/eos/nfs_server.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		log_debug("Log init failed, rc: %d\n", rc);
		goto out;
	}

	rc = config_from_file("libkvsns", config_path, &cfg_items,
							INI_STOP_ON_ERROR, &errors);
	if (rc) {
		printf("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = kvs_init(cfg_items, 0);
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

int nsal_init()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();
	kvs_fid_t fid;

	dassert(kvstor != NULL);

	const char *vfid_str = "<0x780000000000000b:1>";
	rc = kvs_fid_from_str(vfid_str, &fid);
	fid.f_lo = 1;

	kvs_index_open(kvstor, &fid, &kv_index);
	if (rc != 0) {
		printf("Look for idx failed, rc = %d", rc);
		return rc;
	}
	return rc;
}

char *key_list[] = {
	"key_test_1",
	"key_test_2",
	"key_test_3",
	"key_test_4",
	"key_test_5",
	"key_test_6",
	"key_test_7",
	"key_test_8",
	"key_test_9",
	"key_test_10"
};

char *val_list[] = {
	"val_test_1",
	"val_test_2",
	"val_test_3",
	"val_test_4",
	"val_test_5",
	"val_test_6",
	"val_test_7",
	"val_test_8",
	"val_test_9",
	"val_test_10"
};

int set_multiple_key()
{
	int i, rc;
	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor != NULL);
	struct test_key *keys[10];

	for (i = 0; i < 10; ++i) {
		rc = kvs_alloc((void **)&keys[i], sizeof(struct test_key));
		if (rc) {
			goto out;
		}
		keys[i]->type = 'T';
		str256_from_cstr(keys[i]->name, key_list[i], strlen(key_list[i]));
	}
	for (i = 0; i < 10; ++i) {
		rc = kvs_set(&kv_index, keys[i],
		             sizeof(struct test_key), val_list[i],
		             strlen(val_list[i]));
		if (rc) {
			printf("at set i=%d", i);
			break;
		}
	}
out:
	for (i = 0; i < 10; ++i) {
		kvs_free(keys[i]);
	}
	return rc;
}

int iterator_with_prefix()
{
	int rc = 0, i = 0;
	struct kvs_itr *iter;
	bool has_next = true;
	void *key, *value;
	size_t klen, vlen;

	struct test_key prefix;
	prefix.type = 'T';
	size_t len = sizeof(struct test_key) -sizeof(str256_t);

	rc = kvs_itr_find(&kv_index, &prefix, len, &iter);
	if (rc) {
		iter->inner_rc = rc;
		goto out;
	}

	for (i = 0; has_next; ++i) {
		kvs_itr_get(iter, &key, &klen, &value, &vlen);
		printf("key=%s,  value=%s \n", (char *)key, (char*)value);
		rc = kvs_itr_next(iter);
		has_next = !rc;
	}
	if (!has_next) {
		rc = iter->inner_rc == -ENOENT ? 0 : iter->inner_rc;
	} else {
		rc = 0;
	}

out:
	kvs_itr_fini(iter);
	return rc;

}

int iterator_no_prefix()
{
	int rc = 0;
	struct kvs_itr *iter;
	bool has_next = true;
	void *key, *value;
	size_t klen, vlen;

	rc = kvs_itr_find(&kv_index, "", 0, &iter);
	if (rc) {
		iter->inner_rc = rc;
		goto out;
	}
	int i;
    for (i = 0; has_next; ++i) {
		kvs_itr_get(iter, &key, &klen, &value, &vlen);
		printf("key=%s,  value=%s \n", (char *)key, (char*)value);
		rc = kvs_itr_next(iter);
		has_next = !rc;
	}
out:
	kvs_itr_fini(iter);
	return rc;

}

void test_iterator()
{
	int rc = 0;

	rc = set_multiple_key();
	if (rc) {
		printf("Setting multiple keys failed");
		exit(1);
	}
	iterator_no_prefix();
	iterator_with_prefix();
}

int main(int argc, char *argv[])
{
	int rc = 0;

	printf("iter test");
	rc = nsal_start(DEFAULT_CONFIG);
	if (rc) {
		printf("Failed to do nsal_start rc = %d", rc);
	}

	rc = nsal_init();
	if (rc) {
		printf("Failed nsal_init");
	}

	test_iterator();
	printf("All done");

	return 0;
}

