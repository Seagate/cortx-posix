/*
 * Filename: kvtree.h
 * Description: kvtree - Data types and declarations for NSAL kvtree.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 */

#ifndef _KVTREE_H_
#define _KVTREE_H_

#include "str.h"
#include "kvstore.h"
#include "namespace.h"
#include "nsal.h"

/** Version of a kvtree representation. */
typedef enum kvtree_version {
	KVTREE_VERSION_0 = 0,
	KVTREE_VERSION_INVALID,
} kvtree_version_t;

struct kvtree {
	struct namespace *ns;       /* namespace obj */
	struct kvs_idx index;       /* index on which the kvtree works */
	node_id_t root_node_id;     /* Root node id of kvtree */
};

/* Forward declartion*/
struct kvnode;
struct kvnode_basic_attr;

/* KVTree apis */

/** Creates a new kvtree in the given namespace and returns
  * an object that is the in-memory representation of the
  * created tree.
  * A kvtree always have at least one node (root), and
  * users of this API should provide the basic attributes
  * of this node.
  * The created kvtree in-memory object should be
  * finalized with a kvtree_delete() call when it is not
  * needed anymore.
  *
  * @param[in] ns - namespace object for extracting index.
  * @param[in] root_node_attr - Buffer of basic attributes of the root node.
  * @param[in] attr_size - size of the basic attributes buffer
  * @param[out] tree -Allocated kvtree with root.
  *
  * @return 0 if successful, a negative "-errno" value in case of failure
  */
int kvtree_create(struct namespace *ns, const void *root_node_attr,
                  const size_t attr_size, struct kvtree **tree);

/** Deletes the kvtree in-memory object.
 *  The root node associated with this kvtree is also deleted
 *  along with basic attributes of the node from the disk.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvtree_delete(struct kvtree *tree);

/* Initializes the kvtree with namespace obj, default root_id and
 * based on namespace fid opens the index.
 *
 * @param[in] ns - namespace object
 * @param[out] kvtree - initialized kvtree
 * */
int kvtree_init(struct namespace *ns, struct kvtree *tree);

/* Closes the index of kvtree
 *
 * @param[in] tree - kvtree whose index is to be closed.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int kvtree_fini(struct kvtree *tree);

/* Attaches child node to parent node for a given kvtree.
 * Creates a link between parent node and child node based on the child-node name.
 * The caller should validate the child node before attaching its node_id
 * to the tree.
 *
 * @param[in] tree - kvtree to which the child node will be attached.
 * @param[in] parent_id - Node_id of the parent node currently in the kvtree.
 * @param[in] node_id -   Node_id of child node to add to the kvtree.
 * @param[in] node_name - Name of the child node. Here it acts like a link-name.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int kvtree_attach(struct kvtree *tree, const node_id_t *parent_id,
                  const node_id_t *node_id, const str256_t *node_name);

/* Detaches child node from parent node for a given kvtree.
 * Deletes the link between parent node and child node based on the child-node name.
 *
 * @param[in] tree - kvtree from which the child node will be detached.
 * @param[in] parent_id - Node_id of the parent node currently in the kvtree.
 * @param[in] node_name - Name of the child node to remove from the tree.
 *                        Here it acts like a link-name.
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int kvtree_detach(struct kvtree *tree, const node_id_t *parent_id,
                  const str256_t *node_name);

/* Get the child id of a kvnode linked with a parent kvnode in the kvtree.
 *
 * @param[in] tree - kvtree in which the child node is to be found.
 * @param[in] parent_id - node id  of the parent kvnode.
 * @param[in] node_name - Name of the child kvnode to be found.
 * @param[out, optional] node_id - node id of the found child kvnode.
 *
 * @return 0 if successful, otherwise -errno, including -ENOENT
 * if the child kvnode does not exist.
 */
int kvtree_lookup(struct kvtree *tree, const node_id_t *parent_id,
                  const str256_t *node_name, node_id_t *node_id);

/* A callback to be used in kvtree_iter_children.
 * Receives a kvnode at every callback.
 * @retval true continue iteration.
 * @retval false stop iteration.
 */
typedef bool (*kvtree_iter_children_cb)(void *cb_ctx, const char *name,
                                        const struct kvnode *node);

/* Iterate over all the children present in a parent kvnode.
 * Returns a kvnode through the callback function in each iteration.
 *
 * @param[in] tree - kvtree pointer on which iteration is done.
 * @param[in] parent_id - node id  of the parent kvnode.
 * @param[in] cb - Callback function to which kvnode is returned.
 * @param[in] cb_ctx - Callback context
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int kvtree_iter_children(struct kvtree *tree, const node_id_t *parent_id,
                         kvtree_iter_children_cb cb, void *cb_ctx);

 /* Check if the passed kvnode has at least one child kvnode linked into it
 * @param[in] tree - kvtree pointer to check the existence of child nodes .
 * @param[in] parent_id - node id  of the kvnode
 * @param[out] has_children - Flag denoting existence of children
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvtree_has_children(struct kvtree *tree, const node_id_t *parent_id,
                        bool *has_children);
#endif /* _KVTREE_H_ */
