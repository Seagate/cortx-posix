#include <errno.h> /* error no */
#include "kvtree.h"
#include "kvnode.h"
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "md_common.h"

#define DEFAULT_ROOT_ID 2LL

#define ROOT_INIT(__key, __id)             \
{                                          \
    __key.f_hi = __id,                     \
    __key.f_lo = 0;                        \
}

/* Key for parent -> child mapping.
 * Version 1: key = (parent, child_name), value = child.
 * NOTE: This key has variable size.
 */
struct child_node_key {
	md_key_md_t node_md;
	node_id_t parent_id;
	str256_t name;
} __attribute((packed));

#define CHILD_NODE_KEY_INIT(_key, _node_id, _name)      \
{                                                       \
    _key->node_md.k_type = KVTREE_KEY_TYPE_CHILD,       \
    _key->node_md.k_version = KVTREE_VERSION_0,         \
    _key->parent_id = *(_node_id),                      \
    _key->name = *_name;                                \
}

#define CHILD_NODE_PREFIX_INIT(_node_id)            \
{                                                   \
    .node_md = {                                    \
        .k_type = KVTREE_KEY_TYPE_CHILD,            \
        .k_version = KVTREE_VERSION_0,              \
    },                                              \
    .parent_id = *_node_id,                         \
}

/* The size of a child node key prefix */
static const size_t kvtree_child_node_key_psize =
	sizeof(struct child_node_key) - sizeof(str256_t);

/* Dynamic size of a kname object */
static inline size_t kvtree_name_dsize(const str256_t *kname)
{
	/* sizeof (len field) + actual len + size of null-terminator */
	size_t result = sizeof(kname->s_len) + kname->s_len + 1;
	/* Dynamic size cannot be more than the max size of the structure */
	dassert(result <= sizeof(*kname));
	return result;
}

/* Dynamic size of a child node key, i.e. the amount of bytest to be stored in
 * the KVS storage.
 */
static inline
	size_t kvtree_child_node_key_dsize(const struct child_node_key *key)
{
		return kvtree_child_node_key_psize +
		       kvtree_name_dsize(&key->name);
}

/** Pattern size of a child node key, i.e. the size of a child node prefix. */
static const size_t child_node_psize =
	sizeof(struct child_node_key) - sizeof(str256_t);

int kvtree_create(struct namespace *ns, const void *root_node_attr,
                  const size_t attr_size, struct kvtree **tree)
{
	int rc = 0;
	kvs_idx_fid_t ns_fid;
	struct kvs_idx ns_index;
	node_id_t root_id;
	struct kvnode root;

	dassert(root_node_attr);
	dassert(attr_size != 0);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor && ns);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)tree,
	              sizeof(struct kvtree));
	/* Extract fid from namespace and open index*/
	ns_get_fid(ns, &ns_fid);

	RC_WRAP_LABEL(rc, free_tree, kvs_index_open, kvstor, &ns_fid, &ns_index);

	(*tree)->index = ns_index;
	(*tree)->ns = ns;

	ROOT_INIT(root_id, DEFAULT_ROOT_ID);

	/*Creates a node and stores node_attr */
	RC_WRAP_LABEL(rc, close_index, kvnode_init, *tree, &root_id,
	              root_node_attr, attr_size, &root);

	RC_WRAP_LABEL(rc, free_kvnode, kvnode_dump, &root);
	(*tree)->root_node_id = root_id;

free_kvnode:
	kvnode_fini(&root);
close_index:
	kvs_index_close(kvstor, &(*tree)->index);
	if (rc == 0) {
		goto out;
	}
free_tree:
	if (*tree) {
		kvs_free(kvstor, *tree);
	}
out:
	log_debug("kvtree=%p, root_id " NODE_ID_F ", rc=%d", *tree,
	          NODE_ID_P(&root_id), rc);
	return rc;
}

int kvtree_delete(struct kvtree *tree)
{
	int rc = 0;
	kvs_idx_fid_t ns_fid;
	struct kvs_idx ns_index;
	struct kvnode root;

	dassert(tree);
	dassert(tree->ns);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	/* Extract fid from namespace and open index*/
	ns_get_fid(tree->ns, &ns_fid);

	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_fid, &ns_index);

	tree->index = ns_index;
	/* Delete root-node basic attributes for the root-node id */
	RC_WRAP_LABEL(rc, index_close, kvnode_load, tree, &tree->root_node_id, &root);

	RC_WRAP_LABEL(rc, free_kvnode, kvnode_delete, &root);

free_kvnode:
	kvnode_fini(&root);

	kvs_free(kvstor, tree);
index_close:
	kvs_index_close(kvstor, &ns_index);
out:
	log_debug("rc=%d", rc);
	return rc;
}

int kvtree_init(struct namespace *ns, struct kvtree *tree)
{
	int rc = 0;
	kvs_idx_fid_t ns_fid;
	struct kvs_idx ns_index;
	node_id_t root_id;

	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor && ns && tree);

	/* Extract fid from namespace and open index*/
	ns_get_fid(ns, &ns_fid);

	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_fid, &ns_index);

	tree->index = ns_index;
	tree->ns = ns;

	ROOT_INIT(root_id, DEFAULT_ROOT_ID);

	tree->root_node_id = root_id;

out:
	log_debug("kvtree=%p, root_node_id " NODE_ID_F ", rc=%d", tree,
	           NODE_ID_P(&tree->root_node_id), rc);
	return rc;
}

int kvtree_fini(struct kvtree *tree)
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, kvs_index_close, kvstor, &tree->index);
out:
	log_debug("kvtree=%p, rc=%d", tree, rc);
	return rc;
}

int kvtree_attach(struct kvtree *tree, const node_id_t *parent_id,
                  const node_id_t *node_id, const str256_t *node_name)
{
	int rc = 0;
	struct child_node_key *node_key = NULL;

	dassert(tree);
	dassert(parent_id);
	dassert(node_id);
	dassert(node_name);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct child_node_key));

	CHILD_NODE_KEY_INIT(node_key, parent_id, node_name);

	RC_WRAP_LABEL(rc, out, kvs_set, kvstor, &tree->index, node_key,
	              kvtree_child_node_key_dsize(node_key), (void *)node_id,
	              sizeof(node_id_t));

out:
	if (node_key != NULL) {
		kvs_free(kvstor, node_key);
	}
	log_debug("kvtree=%p,parent " NODE_ID_F ",child " NODE_ID_F ",name '"
	           STR256_F"', rc=%d", tree, NODE_ID_P(parent_id),
	           NODE_ID_P(node_id), STR256_P(node_name), rc);
	return rc;
}

int kvtree_detach(struct kvtree *tree, const node_id_t *parent_id,
                  const str256_t *node_name)
{
	int rc = 0;
	struct child_node_key *node_key = NULL;

	dassert(tree);
	dassert(parent_id);
	dassert(node_name);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct child_node_key));

	CHILD_NODE_KEY_INIT(node_key, parent_id, node_name);

	RC_WRAP_LABEL(rc, out, kvs_del, kvstor, &tree->index, node_key,
	              kvtree_child_node_key_dsize(node_key));

out:
	kvs_free(kvstor, node_key);

	log_debug("kvtree=%p,parent " NODE_ID_F ",name '" STR256_F "',rc '%d'",
		  tree, NODE_ID_P(parent_id), STR256_P(node_name), rc);
	return rc;
}

int kvtree_lookup(struct kvtree *tree, const node_id_t *parent_id,
                  const str256_t *node_name, node_id_t *node_id)
{
	int rc = 0;
	struct child_node_key *node_key = NULL;
	node_id_t *val_ptr = NULL;
	node_id_t value = KVNODE_NULL_ID;
	uint64_t val_size = 0;

	dassert(tree);
	dassert(parent_id);
	dassert(node_name);

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&node_key,
	              sizeof(struct child_node_key));

	CHILD_NODE_KEY_INIT(node_key, parent_id, node_name);

	RC_WRAP_LABEL(rc, cleanup, kvs_get, kvstor, &tree->index, node_key,
	              kvtree_child_node_key_dsize(node_key), (void **)&val_ptr,
		      &val_size);

	if (node_id) {
		dassert(val_ptr != NULL);
		dassert(val_size == sizeof(*val_ptr));
		*node_id = *val_ptr;
		value = *val_ptr;
		kvs_free(kvstor, val_ptr);
	}
cleanup:
	kvs_free(kvstor, node_key);

out:
	log_debug("GET " NODE_ID_F ".children." STR256_F "= " NODE_ID_F ", rc=%d",
	          NODE_ID_P(parent_id), STR256_P(node_name), NODE_ID_P(&value), rc);
	return rc;
}

int kvtree_iter_children(struct kvtree *tree, const node_id_t *parent_id,
                         kvtree_iter_children_cb cb, void *cb_ctx)
{
	struct child_node_key prefix = CHILD_NODE_PREFIX_INIT(parent_id);
	int rc;
	size_t klen, vlen;
	bool need_next = true;
	bool has_next = true;
	struct kvs_itr *iter = NULL;
	struct child_node_key *key = NULL;
	node_id_t *value = NULL;
	struct kvnode child_node;
	const char *child_node_name;

	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, kvs_itr_find, kvstor, &tree->index, &prefix,
	              child_node_psize, &iter);

	while (need_next && has_next) {
		kvs_itr_get(kvstor, iter, (void**)&key, &klen, (void **)&value, &vlen);
		/*Child name cannot be empty*/
		dassert(klen > child_node_psize);
		/* The klen is limited by the size of the child node structure. */
		dassert(klen <= sizeof(struct child_node_key));
		dassert(key);

		dassert(vlen == sizeof(*value));
		dassert(value);

		dassert(key->name.s_len != 0);

		child_node_name = key->name.s_str;
		child_node.node_id = *value;

		log_debug("NEXT %s =" NODE_ID_F, child_node_name, NODE_ID_P(value));

		need_next = cb(cb_ctx, child_node_name, &child_node);

		rc = kvs_itr_next(kvstor, iter);
		has_next = (rc == 0);

		log_debug("NEXT_STEP need_next = %d, has_next = %d, rc=%d",
		          (int)need_next, (int)has_next, rc);
	}

	if (!need_next) {
		rc = 0;
	}
out:
	/* Check if iteration was interrupted by an internal KVS error */
	if (rc == -ENOENT) {
		log_debug("No more entries.");
		rc = 0;
	}
	kvs_itr_fini(kvstor, iter);

	log_debug("kvtree=%p, parent " NODE_ID_F ", rc=%d", tree,
	           NODE_ID_P(parent_id), rc);
	return rc;
}
