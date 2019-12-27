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

int kvstore_init(struct kvstore *kvstore, char *type,
		 struct collection_item *cfg,
		 int flags, struct kvstore_ops *kvstore_ops,
		 struct kvstore_index_ops *index_ops,
		 struct kvstore_kv_ops *kv_ops)
{
	int rc;
	assert(kvstore && type && kvstore_ops && index_ops && kv_ops);

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

