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

static struct kvstore g_kvstore;

struct kvstore *kvstore_get(void)
{
	return &g_kvstore;
}

int kvstore_init(struct kvstore *kvstore, char *type,
		 struct collection_item *cfg,
		 int flags, struct kvstore_ops *kvstore_ops,
		 struct kvstore_index_ops *index_ops,
		 struct kvstore_kv_ops *kv_ops)
{
	int rc;
	assert(kvstore && type && kvstore_ops && index_ops && kv_ops);

	kvstore->kvstore_ops = kvstore_ops;
	rc = kvstore_ops->init(cfg);
	if (rc)
		return rc;

	kvstore->type = type;
	kvstore->cfg = cfg;
	if (!kvstore_ops->alloc && !kvstore_ops->free) {
		kvstore_ops->alloc = kvstore_alloc;
		kvstore_ops->free = kvstore_free;
	}
	kvstore->index_ops = index_ops;
	kvstore->kv_ops = kv_ops;
	kvstore->flags = flags;

	return 0;
}

int kvstore_fini(struct kvstore *kvstore)
{
	assert(kvstore && kvstore->kvstore_ops && kvstore->kvstore_ops->fini);

	return kvstore->kvstore_ops->fini();
}

int kvstore_alloc(void **ptr, uint64_t size)
{
	*ptr = malloc(size);
	if (*ptr == NULL)
		return -ENOMEM;
	return 0;
}

void kvstore_free(void *ptr)
{
	free(ptr);
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
