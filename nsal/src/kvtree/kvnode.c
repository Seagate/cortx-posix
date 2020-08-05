#include "kvtree.h"
#include "kvnode.h"
#include "kvnode_internal.h"
#include <string.h>
#include "md_common.h"
#include "utils.h"
#include "common/helpers.h" /* RC_WRAP_LABEL */

struct kvnode_key {
	md_key_md_t node_md;
	node_id_t node_id;
} __attribute((packed));

#define KVNODE_KEY_INIT(_key, _node_id, _ktype)         \
{                                                       \
    _key->node_md.k_type = _ktype;                      \
    _key->node_md.k_version = KVTREE_VERSION_0;         \
    _key->node_id = _node_id;                           \
}

struct kvnode_sys_attr_key {
	md_key_md_t node_md;
	node_id_t node_id;
	uint8_t attr_id;
} __attribute((packed));

#define KVNODE_SYS_ATTR_KEY_INIT(_key, _node_id, _attr_id)        \
{                                                                 \
    _key->node_md.k_type = KVTREE_KEY_TYPE_SYSTEM_ATTR;           \
    _key->node_md.k_version = KVTREE_VERSION_0;                   \
    _key->node_id = _node_id;                                     \
    _key->attr_id = _attr_id;                                     \
}

int kvnode_init(struct kvtree *tree, const node_id_t *node_id, const void *attr,
                const uint16_t attr_size, struct kvnode *node)
{
	int rc = 0;
	size_t tot_size = 0;
	struct kvnode_basic_attr *node_attr = NULL;
	struct kvstore *kvstor = NULL;

	dassert(tree);
	dassert(node_id);
	dassert(node);
	dassert(attr && attr_size);

	kvstor = kvstore_get();
	dassert(kvstor);

	tot_size = sizeof(struct kvnode_basic_attr) + attr_size;

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_attr, tot_size);

	memcpy(node_attr->attr, attr, attr_size);
	node_attr->size = attr_size;

	node->tree = tree;
	node->node_id = *node_id;
	node->basic_attr = node_attr;

out:
	log_debug("kvtree=%p, node_id " NODE_ID_F ", size of node attr = %zu, "
	           "rc=%d", tree, NODE_ID_P(node_id), tot_size, rc);
	return rc;
}

int kvnode_dump(const struct kvnode *node)
{
	struct kvnode_key *node_key = NULL;
	int rc = 0;
	size_t tot_size = 0;

	dassert(node);
	dassert(node->basic_attr);
	dassert(node->basic_attr->size != 0);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	tot_size = sizeof(struct kvnode_basic_attr) + node->basic_attr->size;

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_KEY_INIT(node_key, node->node_id, KVTREE_KEY_TYPE_BASIC_ATTR);

	RC_WRAP_LABEL(rc, out, kvs_set, kvstor, &node->tree->index, node_key,
	             sizeof(struct kvnode_key), node->basic_attr, tot_size);

out:
	if (node_key != NULL) {
		kvs_free(kvstor, node_key);
	}
	log_debug("kvtree=%p, node_id " NODE_ID_F ", total ba size=%zu, rc=%d",
		  node->tree, NODE_ID_P(&node->node_id), tot_size, rc);
	return rc;
}

int kvnode_load(struct kvtree *tree, const node_id_t *node_id,
                struct kvnode *node)
{
	struct kvnode_key *node_key = NULL;
	int rc = 0;
	size_t  val_size = 0;
	struct kvnode_basic_attr *val = NULL;

	dassert(tree);
	dassert(node_id);
	dassert(node);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_KEY_INIT(node_key, *node_id, KVTREE_KEY_TYPE_BASIC_ATTR);

	RC_WRAP_LABEL(rc, free_key, kvs_get, kvstor, &tree->index, node_key,
	              sizeof(struct kvnode_key), (void **)&val, &val_size);

	dassert(val);
	dassert(val_size == (sizeof(struct kvnode_basic_attr) + val->size));

	node->tree = tree;
	node->node_id = *node_id;
	node->basic_attr = val;

free_key:
	kvs_free(kvstor, node_key);
out:
	log_debug("kvtree=%p, node_id " NODE_ID_F ", rc=%d", tree,
	           NODE_ID_P(node_id), rc);
	return rc;
}

int kvnode_delete(const struct kvnode *node)
{
	struct kvnode_key *node_key = NULL;
	int rc = 0;

	dassert(node);
	dassert(node->tree);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_KEY_INIT(node_key, node->node_id, KVTREE_KEY_TYPE_BASIC_ATTR);

	RC_WRAP_LABEL(rc, out, kvs_del, kvstor, &node->tree->index, node_key,
	              sizeof(struct kvnode_key));

out:
	if (node_key != NULL) {
		kvs_free(kvstor, node_key);
	}
	log_debug("kvtree=%p, node_id " NODE_ID_F ", rc=%d", node->tree,
	          NODE_ID_P(&node->node_id), rc);
	return rc;
}

void kvnode_fini(struct kvnode *node)
{
	dassert(node);
	if (node->tree) {
		dassert(node->basic_attr);

		struct kvstore *kvstor = kvstore_get();
		dassert(kvstor);

		kvs_free(kvstor, node->basic_attr);

		node->tree = NULL;;
		node->node_id = KVNODE_NULL_ID;
		node->basic_attr = NULL;
	}
}

int kvnode_set_sys_attr(const struct kvnode *node, const int key,
                        const buff_t value)
{
	int rc = 0;
	struct kvnode_sys_attr_key *node_key = NULL;

	dassert(node);
	dassert(node->tree);
	dassert(value.buf);
	dassert(value.len != 0);

	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_sys_attr_key));

	KVNODE_SYS_ATTR_KEY_INIT(node_key, node->node_id, key);

	RC_WRAP_LABEL(rc, out, kvs_set, kvstor, &node->tree->index, node_key,
	             sizeof(struct kvnode_sys_attr_key), value.buf, value.len);

out:
	if (node_key != NULL) {
		kvs_free(kvstor, node_key);
	}
	log_debug("kvtree=%p, node_id " NODE_ID_F ", System Attr id=%d, value"
	          " size=%lu, rc=%d", node->tree, NODE_ID_P(&node->node_id), key,
	          value.len, rc);
	return rc;
}

int kvnode_get_sys_attr(const struct kvnode *node, const int key,
                        buff_t *value)
{
	int rc = 0;
	struct kvnode_sys_attr_key *node_key = NULL;

	dassert(node);
	dassert(node->tree);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_sys_attr_key));

	KVNODE_SYS_ATTR_KEY_INIT(node_key, node->node_id, key);

	RC_WRAP_LABEL(rc, out, kvs_get, kvstor, &node->tree->index, node_key,
	              sizeof(struct kvnode_sys_attr_key), &value->buf, &value->len);

	dassert(value->buf != NULL);
	dassert(value->len != 0);

out:
	if (node_key != NULL) {
		kvs_free(kvstor, node_key);
	}
	log_debug("kvtree=%p, node_id " NODE_ID_F ", System Attr id=%d, rc=%d",
	          node->tree, NODE_ID_P(&node->node_id), key, rc);
	return rc;
}

int kvnode_del_sys_attr(const struct kvnode *node, const int key)
{
	int rc = 0;
	struct kvnode_sys_attr_key *node_key = NULL;

	dassert(node);
	dassert(node->tree);

	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct kvnode_key));

	KVNODE_SYS_ATTR_KEY_INIT(node_key, node->node_id, key);

	RC_WRAP_LABEL(rc, out, kvs_del, kvstor, &node->tree->index, node_key,
	              sizeof(struct kvnode_sys_attr_key));

out:
	if (node_key != NULL) {
		kvs_free(kvstor, node_key);
	}
	log_debug("kvtree=%p, node_id " NODE_ID_F ", System Attr id=%d, rc=%d",
	          node->tree, NODE_ID_P(&node->node_id), key, rc);
	return rc;
}

uint16_t kvnode_get_basic_attr_buff(const struct kvnode *node, void **attr_buff)
{
	struct kvnode_basic_attr *basic_attr;

	dassert(node);
	dassert(node->basic_attr);

	basic_attr = node->basic_attr;
	*attr_buff = (void *)basic_attr->attr;

	return basic_attr->size;
}
