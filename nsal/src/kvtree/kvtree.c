#include "kvtree.h"
#include "kvnode.h"
#include "common/helpers.h" /* RC_WRAP_LABEL */

#define DEFAULT_ROOT_ID 2LL

#define ROOT_INIT(__key, __id)             \
{                                          \
	__key.f_hi = __id,                     \
	__key.f_lo = 0;                      \
}

int kvtree_create(struct namespace *ns, struct kvnode_info *root_node_info,
                  struct kvtree **kvtree)
{
	int rc = 0;
	kvs_idx_fid_t ns_fid;
	struct kvs_idx ns_index;
	node_id_t root_id;
	struct kvstore *kvstor = kvstore_get();
	struct kvnode root;

	dassert(kvstor && ns);

	dassert(root_node_info);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)kvtree,
	              sizeof(struct kvtree));
	/* Extract fid from namespace and open index*/
	ns_get_fid(ns, &ns_fid);

	RC_WRAP_LABEL(rc, free_kvtree, kvs_index_open, kvstor, &ns_fid, &ns_index);

	(*kvtree)->index = ns_index;
	(*kvtree)->ns = ns;

	ROOT_INIT(root_id, DEFAULT_ROOT_ID);

	/*Creates a node and stores node_info */
	RC_WRAP_LABEL(rc, close_index, kvnode_create, *kvtree, root_id, 
	              root_node_info, &root);

	(*kvtree)->root_node_id = root_id;

close_index:
	kvs_index_close(kvstor, &((*kvtree)->index));
	if (rc == 0) {
		goto out;
	}
free_kvtree:
	if (*kvtree) {
		kvs_free(kvstor, *kvtree);
	}
out:
	return rc;
}

int kvtree_delete(struct kvtree *kvtree)
{
	int rc = 0;
	kvs_idx_fid_t ns_fid;
	struct kvs_idx ns_index;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);
	dassert(kvtree->ns);

	/* Extract fid from namespace and open index*/
	ns_get_fid(kvtree->ns, &ns_fid);

	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_fid, &ns_index);

	kvtree->index = ns_index;
	/* Delete node_info for the node id */
	RC_WRAP_LABEL(rc, out, kvnode_delete, kvtree, &(kvtree->root_node_id));

	rc = kvs_index_close(kvstor, &(kvtree->index));

	kvs_free(kvstor, kvtree);

out:
	return rc;
}

int kvtree_init(struct namespace *ns, struct kvtree *kvtree)
{
	int rc = 0;
	kvs_idx_fid_t ns_fid;
	struct kvs_idx ns_index;
	node_id_t root_id;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor && ns && kvtree);

	/* Extract fid from namespace and open index*/
	ns_get_fid(ns, &ns_fid);

	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_fid, &ns_index);

	kvtree->index = ns_index;
	kvtree->ns = ns;

	ROOT_INIT(root_id, DEFAULT_ROOT_ID);

	kvtree->root_node_id = root_id;
	goto out;

out:
	return rc;
}

int kvtree_fini(struct kvtree *kvtree)
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	rc = kvs_index_close(kvstor, &(kvtree->index));

	return rc;
}
