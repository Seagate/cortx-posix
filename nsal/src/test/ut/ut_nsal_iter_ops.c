/*
 * Filename:         ut_nsal_iter_ops.c
 * Description:
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
 
#include <stdio.h>
#include <stdlib.h>
#include <ini_config.h>
#include "kvstore.h"
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <common/log.h>
#include "str.h"
#include <ut.h>

#define DEFAULT_CONFIG "/etc/cortx/cortxfs.conf"
#define CONF_FILE "/tmp/eos-fs/build-nsal/test/ut/ut_nsal.conf"
/* key_list and val_list needs to be in sync with below config */
#define MAX_NUM_KEY_VALUE_PAIR 10

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

	rc = log_init("/var/log/cortx/fs/efs.log", LEVEL_DEBUG);
	if (rc) {
		rc = -EINVAL;
		printf("Log init failed, rc: %d\n", rc);
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

	log_debug("rc=%d nsal_start done", rc);

out:
	if (rc) {
		free_ini_config_errors(errors);
	}

	return rc;
}

int iter_init(void)
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();
	kvs_idx_fid_t fid;
	struct collection_item *item;
	const char *fid_str;

	dassert(kvstor != NULL);

	rc = get_config_item("mero", "kvs_fid", cfg_items, &item);
	if (rc) {
		log_err("get_config_item failed, rc = %d", rc);
		goto out;
	}

	fid_str = get_string_config_value(item, NULL);
	if (fid_str == NULL) {
		log_err("get_string_config_value failed, rc = %d", rc);
		goto out;
	}

	rc = kvs_fid_from_str(fid_str, &fid);
	if (rc) {
		log_err("kvs_fid_from_str failed, rc = %d", rc);
		goto out;
	}

	rc = kvs_index_open(kvstor, &fid, &kv_index);
	if (rc) {
		log_err("Look for idx failed, rc = %d", rc);
		goto out;
	}

	log_debug("rc=%d iter_init done", rc);

out:
	return rc;
}

int iter_fini(void)
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);
	rc = kvs_index_close(kvstor, &kv_index);
	if (rc != 0) {
		log_err("kvs_index_close failed, rc = %d", rc);
	}

	rc = kvs_fini(kvstor);
	if (rc != 0) {
		log_err("kvs_fini failed, rc = %d", rc);
	}

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

bool test_iter_validate_key_val(struct test_key *key, size_t klen,
				 char *value, size_t vlen)
{
	int index;
	bool match_found = false;

	if (key->type == 'T') {
		/* One of the key and respective value shoud match */
		for (index = 0; index < MAX_NUM_KEY_VALUE_PAIR; index++) {
			if (strcmp(key->name.s_str, key_list[index]) == 0) {
				if (sizeof(struct test_key) != klen) {
					break;
				}

				if(strlen(val_list[index]) != (vlen - 1)) {
					break;
				}

				if (strcmp(val_list[index], value) != 0) {
					break;
				}

				match_found = true;
				break;
			}
		}
	}
	return match_found;
}

int set_multiple_key(void)
{
	int i, rc;
	struct test_key *keys[MAX_NUM_KEY_VALUE_PAIR];
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	for (i = 0; i < MAX_NUM_KEY_VALUE_PAIR; ++i) {
		rc = kvs_alloc(kvstor, (void **)&keys[i],
			       sizeof(struct test_key));
		if (rc) {
			goto out;
		}
		keys[i]->type = 'T';
		str256_from_cstr(keys[i]->name, key_list[i],
				 strlen(key_list[i]));
	}

	for (i = 0; i < MAX_NUM_KEY_VALUE_PAIR; ++i) {
		rc = kvs_set(kvstor, &kv_index, keys[i],
		             sizeof(struct test_key), val_list[i],
		             strlen(val_list[i])+1);
		if (rc) {
			break;
		}
	}

out:
	for (i = 0; i < MAX_NUM_KEY_VALUE_PAIR; ++i) {
		if (keys[i]) {
			kvs_free(kvstor, keys[i]);
		}
	}

	return rc;
}


void test_iterator_with_prefix(void)
{
	int rc = 0;
	int matching_key_val = 0;
	struct kvs_itr *iter;
	bool has_next = true;
	void *key, *value;
	size_t klen, vlen;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	struct test_key prefix;
	prefix.type = 'T';
	size_t len = sizeof(struct test_key) - sizeof(str256_t);

	rc = kvs_itr_find(kvstor, &kv_index, &prefix, len, &iter);
	if (rc) {
		iter->inner_rc = rc;
		goto out;
	}

	while (has_next) {
		kvs_itr_get(kvstor, iter, &key, &klen, &value, &vlen);

		if (test_iter_validate_key_val(key, klen, value, vlen)) {
			matching_key_val++;
		}

		printf("key_type=%c, key_name=%s, value=%s \n",
		       ((struct test_key*)key)->type,
		       ((struct test_key*)key)->name.s_str, (char*)value);
		rc = kvs_itr_next(kvstor, iter);
		has_next = (rc == 0);
	}

	ut_assert_int_equal(matching_key_val, MAX_NUM_KEY_VALUE_PAIR);

out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	kvs_itr_fini(kvstor, iter);
	printf("with_prefix rc = %d\n", rc);
	ut_assert_int_equal(rc,0);
}

void test_iterator_no_prefix(void)
{
	int rc = 0;
	int matching_key_val = 0;
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

		if (test_iter_validate_key_val(key, klen, value, vlen)) {
			matching_key_val++;
		}

		printf("key_type=%c, key_name=%s, value=%s \n",
		       ((struct test_key*)key)->type,
		       ((struct test_key*)key)->name.s_str, (char*)value);

		rc = kvs_itr_next(kvstor, iter);
		has_next = (rc == 0);
	}

	ut_assert_int_equal(matching_key_val, MAX_NUM_KEY_VALUE_PAIR);

out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	kvs_itr_fini(kvstor, iter);
	printf("no prefix rc = %d\n", rc);
	ut_assert_int_equal(rc,0);
}

int main(void)
{
	int rc = 0;
	char *test_logs = "/var/log/eos/test/ut/ut_nsal.logs";

	printf("Iterator test\n");

	rc = ut_load_config(CONF_FILE);
	if (rc != 0) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_logs = ut_get_config("nsal", "log_path", test_logs);

	rc = ut_init(test_logs);

	rc = nsal_start(DEFAULT_CONFIG);
	if (rc) {
		printf("Failed to do nsal_start rc = %d", rc);
		goto out;
	}

	rc = iter_init();
	if (rc) {
		log_err("Failed iter_init");
		goto out;
	}

	rc = set_multiple_key();
	if (rc) {
		printf("Setting multiple keys failed");
		goto out;
	}

	struct test_case test_list[] = {
		ut_test_case(test_iterator_with_prefix, NULL, NULL),
		ut_test_case(test_iterator_no_prefix, NULL, NULL)
	};

	int test_count =  sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	rc = iter_fini();
	if (rc) {
		log_err("Failed iter_fini");
		goto out;
	}
out:
	ut_fini();

	ut_summary(test_count, test_failed);

	free(test_logs);
end:
	return rc;
}
