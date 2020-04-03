#include "kvtree.h"
#include "kvnode.h"
#include <string.h>
#include "md_common.h"
#include "common/helpers.h" /* RC_WRAP_LABEL */

struct kvnode_key {
	md_key_md_t node_md;
	node_id_t node_id;
} __attribute((packed));

#define KVNODE_KEY_INIT(_key, _node_id, _ktype)    \
{                                                       \
	_key->node_md.k_type = _ktype,                      \
	_key->node_md.k_version = KVTREE_VERSION_0,         \
	_key->node_id = _node_id;                           \
}

int kvnode_create(struct kvtree *kvtree, node_id_t node_id,
                  struct kvnode_info *node_info, struct kvnode *node)
{
	int rc = 0;

	dassert(kvtree);
	dassert(node);

	node->kvtree = kvtree;
	node->node_id = node_id;
	node->node_info = node_info;
	kvnode_info_write(node);

	return rc;
}

int kvnode_info_alloc(const void *buff, const size_t size,
                      struct kvnode_info **ret_node_info)
{
	struct kvnode_info *node_info;
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();
	dassert(buff && size);

	size_t tot_size = sizeof(struct kvnode_info) + size;
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_info, tot_size);

	memcpy(node_info->info, buff, size);
	node_info->size = size;

	*ret_node_info = node_info;
out:
	return rc;
}

void kvnode_info_free(struct kvnode_info *node_info)
{
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	kvs_free(kvstor, node_info);
}

int kvnode_info_write(struct kvnode *node)
{
	struct kvnode_key *node_key = NULL;
	int rc = 0;
	size_t tot_size = sizeof(struct kvnode_info) + node->node_info->size;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_KEY_INIT(node_key, node->node_id, KVTREE_KEY_TYPE_NODE_INFO);

	rc = kvs_set(kvstor, &(node->kvtree->index), node_key,
	             sizeof(struct kvnode_key), node->node_info, tot_size);
	kvs_free(kvstor, node_key);
out:
	return rc;
}

int kvnode_info_read(struct kvnode *node)
{
	struct kvnode_key *node_key = NULL;
	int rc = 0;
	size_t  val_size = 0;

	struct kvnode_info *info = NULL;

	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_KEY_INIT(node_key, node->node_id, KVTREE_KEY_TYPE_NODE_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_get, kvstor, &(node->kvtree->index), node_key,
	             sizeof(struct kvnode_key), (void **)&info, &val_size);

	node->node_info = info;

	node->node_info->size = val_size - sizeof(struct kvnode_info);

free_key:
		kvs_free(kvstor, node_key);
out:
	return rc;
}

int kvnode_info_remove(struct kvtree *kvtree, node_id_t *node_id)
{
	struct kvnode_key *node_key = NULL;
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_KEY_INIT(node_key, *node_id, KVTREE_KEY_TYPE_NODE_INFO);

	rc = kvs_del(kvstor, &(kvtree->index), node_key, sizeof(struct kvnode_key));

	kvs_free(kvstor, node_key);
out:
	return rc;
}

int kvnode_delete(struct kvtree *kvtree, node_id_t *node_id)
{
	return kvnode_info_remove(kvtree, node_id);
}
