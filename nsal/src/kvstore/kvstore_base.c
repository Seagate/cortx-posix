/*
 * Filename:         kvstore_base.c
 * Description:      Contains implementation of basic KVStore
 * 		     framework APIs.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 Contains implementation of basic kvstore framework APIs.
*/

#include "kvstore.h"
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <eos/eos_kvstore.h>
#include "redis/redis_kvstore.h"
#include <string.h>
#include <common/helpers.h>
#include <common/log.h>

#define KVSTORE "kvstore"
#define TYPE "type"

static struct kvstore g_kvstore;

struct kvstore_module {
    char *type;
    struct kvstore_ops *ops;
};

static struct kvstore_module kvstore_modules[] = {
    { "eos", &eos_kvs_ops },
#ifdef	WITH_REDIS
    { "redis", &redis_kvs_ops },
#endif
    { NULL, NULL },
};

struct kvstore *kvstore_get(void)
{
	return &g_kvstore;
}

int kvs_init(struct kvstore *kvstore, struct collection_item *cfg)

{
	int rc = 0, i;
	char *kvstore_type = NULL;
	struct collection_item *item = NULL;

	dassert(kvstore && cfg);

	RC_WRAP(get_config_item, KVSTORE, TYPE, cfg, &item);
	if (item == NULL) {
		log_err("KVStore type not specified\n");
		rc = -EINVAL;
		goto out;
	}
	kvstore_type = get_string_config_value(item, NULL);

	for (i = 0; kvstore_modules[i].type != NULL; ++i) {
		if (strncmp(kvstore_type, kvstore_modules[i].type,
		    strlen(kvstore_type)) == 0) {
			kvstore->kvstore_ops = kvstore_modules[i].ops;
			break;
		}
	}
	if (kvstore_modules[i].type == NULL) {
		log_err("Invalid kvstore type %s", kvstore_type);
		rc = -EINVAL;
		goto out;
	}

	dassert(kvstore->kvstore_ops->init != NULL);
	rc = kvstore->kvstore_ops->init(cfg);
	if (rc) {
		goto out;
	}

	kvstore->type = kvstore_type;
out:
	return rc;
}

int kvs_fini(struct kvstore *kvstore)
{
	dassert(kvstore && kvstore->kvstore_ops && kvstore->kvstore_ops->fini);

	return kvstore->kvstore_ops->fini();
}

int kvs_fid_from_str(const char *fid_str, kvs_fid_t *out_fid)
{
	return eos_kvs_fid_from_str(fid_str, out_fid);
}

int kvs_alloc(struct kvstore *kvstore, void **ptr, size_t size)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->alloc(ptr, size);
}

void kvs_free(struct kvstore *kvstore, void *ptr)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->free(ptr);
}

int kvs_begin_transaction(struct kvstore *kvstore, struct kvs_idx *index)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->begin_transaction(index);
}

int kvs_end_transaction(struct kvstore *kvstore, struct kvs_idx *index)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->end_transaction(index);
}

int kvs_discard_transaction(struct kvstore *kvstore, struct kvs_idx *index)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->discard_transaction(index);
}

int kvs_index_create(struct kvstore *kvstore, const kvs_fid_t *fid,
	                 struct kvs_idx *index)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->index_create(fid, index);
}

int kvs_index_delete(struct kvstore *kvstore, const kvs_fid_t *fid)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->index_delete(fid);
}

int kvs_index_open(struct kvstore *kvstore, const kvs_fid_t *fid,
			       struct kvs_idx *index)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->index_open(fid, index);
}

int kvs_index_close(struct kvstore *kvstore, struct kvs_idx *index)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->index_close(index);
}

int kvs_get(struct kvstore *kvstore, struct kvs_idx *index, void *k, const size_t klen,
	        void **v, size_t *vlen)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->get_bin(index, k, klen, v, vlen);
}

int kvs_set(struct kvstore *kvstore, struct kvs_idx *index, void *k, const size_t klen,
	        void *v, const size_t vlen)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->set_bin(index, k, klen, v, vlen);
}

int kvs_del(struct kvstore *kvstore, struct kvs_idx *index, const void *k,
            size_t klen)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->del_bin(index, k, klen);
}

/* Key-Value iterator API */
int kvs_itr_find(struct kvstore *kvstore, struct kvs_idx *index, void *prefix,
                  const size_t prefix_len,
                  struct kvs_itr **iter)
{
	int rc = 0;

	dassert(kvstore);

	rc = kvstore->kvstore_ops->alloc((void **)iter, sizeof(struct kvs_itr));
	if (rc) {
		return rc;
	}
	(*iter)->idx.index_priv = index->index_priv;
	(*iter)->prefix.buf = prefix;
	(*iter)->prefix.len = prefix_len;

	return kvstore->kvstore_ops->kv_find(*iter);
}

int kvs_itr_next(struct kvstore *kvstore, struct kvs_itr *iter)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->kv_next(iter);
}

void kvs_itr_fini(struct kvstore *kvstore, struct kvs_itr *iter)
{
	dassert(kvstore);
	kvstore->kvstore_ops->kv_fini(iter);
	kvstore->kvstore_ops->free(iter);
}

void kvs_itr_get(struct kvstore *kvstore, struct kvs_itr *iter, void **key,
                 size_t *klen, void **val, size_t *vlen)
{
	dassert(kvstore);
	return kvstore->kvstore_ops->kv_get(iter, key, klen, val, vlen);
}

int kvpair_alloc(struct kvpair **kv)
{
	struct kvstore *kvstor = kvstore_get();
	int rc = 0;

	dassert(*kv == NULL);
	rc = kvstor->kvstore_ops->alloc((void **)kv, sizeof(struct kvpair));
	return rc;
}

void kvpair_free(struct kvpair *kv)
{
	struct kvstore *kvstor = kvstore_get();
	kvstor->kvstore_ops->free(kv);
}

void kvpair_init(struct kvpair *kv, void *key, const size_t klen,
                 void *val, const size_t vlen)
{
	dassert(kv);
	dassert(key && klen && val && vlen);

	kv->key.buf = key;
	kv->key.len = klen;

	kv->val.buf = val;
	kv->val.len = vlen;
}

int kvgroup_init(struct kvgroup *kv_grp, const uint32_t size)
{
	struct kvstore *kvstor = kvstore_get();
	int rc = 0;
	struct kvpair *temp_list = NULL;

	dassert(kv_grp->kv_list == NULL);
	rc = kvstor->kvstore_ops->alloc((void **)&temp_list,
	                                sizeof(struct kvpair *) * size);
	if (rc) {
		goto out;
	}
	kv_grp->kv_list = &temp_list;
	kv_grp->kv_max = size;
	kv_grp->kv_count = 0;
out:
	return rc;
}

int kvgroup_add(struct kvgroup *kv_grp, struct kvpair *kv)
{
	if (kv_grp->kv_count == kv_grp->kv_max)
		return -ENOMEM;
	kv_grp->kv_list[kv_grp->kv_count++] = kv;
	return 0;
}

void kvgroup_fini(struct kvgroup *kv_grp)
{
	struct kvstore *kvstor = kvstore_get();
	uint32_t i;
	if (kv_grp != NULL) {
		if (kv_grp->kv_list != NULL) {
			for (i = 0; i < kv_grp->kv_count; ++i) {
				kvpair_free(kv_grp->kv_list[i]);
				kv_grp->kv_list[i] = NULL;
			}
		}
		kvstor->kvstore_ops->free(kv_grp->kv_list);
		kv_grp->kv_list = NULL;
	}
}

int kvgroup_kvpair_get(struct kvgroup *kv_grp, const int index,
                        void **value, size_t *vlen)
{
	if (index >= kv_grp->kv_count)
		return -ENOMEM;
	struct kvpair *kv = kv_grp->kv_list[index];
	*value = kv->val.buf;
	*vlen = kv->val.len;
	if (*value == NULL && *vlen == 0) {
		return -EINVAL;
	}
	return 0;
}
