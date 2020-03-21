#include <stdio.h>
#include <stdlib.h>
#include <ini_config.h>
#include "eos/eos_kvstore.h"
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <common/log.h>
#include "str.h"
#include <ut.h>

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
	struct kvstore *kvstore = kvstore_get();
	int rc = 0;

	dassert(kvstore != NULL);

	rc = log_init("/var/log/eos/efs/efs.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		log_debug("Log init failed, rc: %d\n", rc);
		goto out;
	}

	rc = config_from_file("libkvsns", config_path, &cfg_items,
							INI_STOP_ON_ERROR, &errors);
	if (rc) {
		log_err("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = kvs_init(kvstore, cfg_items);
	if (rc) {
		log_err("Failed to do kvstore init rc = %d", rc);
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

int iter_init()
{
        int rc = 0;
        struct kvstore *kvstor = kvstore_get();
        kvs_idx_fid_t fid;
        struct collection_item *item;
        const char *fid_str;

        dassert(kvstor != NULL);

        rc = get_config_item("mero", "kvs_fid", cfg_items, &item);
        fid_str = get_string_config_value(item, NULL);
        rc = kvs_fid_from_str(fid_str, &fid);

        rc = kvs_index_open(kvstor, &fid, &kv_index);
        if (rc != 0) {
                log_err("Look for idx failed, rc = %d", rc);
                return rc;
        }
        return rc;
}

int iter_fini()
{
        int rc = 0;
        struct kvstore *kvstor = kvstore_get();

        dassert(kvstor != NULL);
        rc = kvs_index_close(kvstor, &kv_index);

        kvs_fini(kvstor);
        free_ini_config_errors(cfg_items);

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
		rc = kvs_alloc(kvstor, (void **)&keys[i], sizeof(struct test_key));
		if (rc) {
			goto out;
		}
		keys[i]->type = 'T';
		str256_from_cstr(keys[i]->name, key_list[i], strlen(key_list[i]));
	}
	for (i = 0; i < 10; ++i) {
		rc = kvs_set(kvstor, &kv_index, keys[i],
		             sizeof(struct test_key), val_list[i],
		             strlen(val_list[i]));
		if (rc) {
			break;
		}
	}
out:
	for (i = 0; i < 10; ++i) {
		kvs_free(kvstor, keys[i]);
	}
	return rc;
}

void test_iterator_with_prefix()
{
	int rc = 0;
	struct kvs_itr *iter;
	bool has_next = true;
	void *key, *value;
	size_t klen, vlen;
	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor != NULL);

	struct test_key prefix;
	prefix.type = 'T';
	size_t len = sizeof(struct test_key) -sizeof(str256_t);

	rc = kvs_itr_find(kvstor, &kv_index, &prefix, len, &iter);
	if (rc) {
		iter->inner_rc = rc;
		goto out;
	}

	while (has_next) {
		kvs_itr_get(kvstor, iter, &key, &klen, &value, &vlen);

		printf("key=%s,  value=%s \n", (char *)key, (char*)value);
		rc = kvs_itr_next(kvstor, iter);
		has_next = (rc == 0);
	}
out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	kvs_itr_fini(kvstor, iter);
	printf("with_prefix rc = %d\n", rc);
	ut_assert_int_equal(rc,0);
}

void test_iterator_no_prefix()
{
	int rc = 0;
	struct kvs_itr *iter;
	bool has_next = true;
	void *key, *value;
	size_t klen, vlen;
	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor != NULL);

	rc = kvs_itr_find(kvstor, &kv_index, "", 0, &iter);
	if (rc) {
		iter->inner_rc = rc;
		goto out;
	}

	while (has_next) {
		kvs_itr_get(kvstor, iter, &key, &klen, &value, &vlen);
		printf("key=%s,  value=%s \n", (char *)key, (char*)value);
		rc = kvs_itr_next(kvstor, iter);
		has_next = (rc == 0);
	}
out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	kvs_itr_fini(kvstor, iter);
	printf("no prefix rc = %d\n", rc);
	ut_assert_int_equal(rc,0);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char *test_logs = "/var/log/eos/test.logs";

	if (argc > 1) {
		test_logs = argv[1];
	}
	printf("Iterator test\n");

	rc = ut_init(test_logs);

	rc = nsal_start(DEFAULT_CONFIG);
	if (rc) {
		log_err("Failed to do nsal_start rc = %d", rc);
		goto out;
	}

	rc = iter_init(cfg_items);
	if (rc) {
		log_err("Failed iter_init");
		goto out;
	}

	rc = set_multiple_key();
	if (rc) {
		printf("Setting multiple keys failed");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_iterator_with_prefix),
		ut_test_case(test_iterator_no_prefix)
	};

	int test_count = 2;
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	rc = iter_fini();
	if (rc) {
		log_err("Failed iter_fini");
		goto out;
	}
out:
	ut_fini();
	printf("Tests failed = %d", test_failed);

	return rc;
}
