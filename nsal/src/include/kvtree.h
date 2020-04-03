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

typedef obj_id_t node_id_t;

/** Version of a kvtree representation. */
typedef enum kvtree_version {
	KVTREE_VERSION_0 = 0,
	KVTREE_VERSION_INVALID,
} kvtree_version_t;

/* KVTree key types associated with particular version of kvtree. */
typedef enum kvtree_key_type {
	KVTREE_KEY_TYPE_NODE_INFO = 20,
	KVTREE_KEY_TYPE_CHILD,
	KVTREE_KEY_TYPE_SYMLINK,
	KVTREE_KEY_TYPE_INVALID,
} kvtree_key_type_t;

struct kvtree {
	struct namespace *ns;       /* namespace obj */
	struct kvs_idx index;       /* index on which the kvtree works */
	node_id_t root_node_id;     /* Root node id of kvtree */
};

struct kvnode_info;

/* KVTree apis */

/** Creates a new kvtree by allocating memory and storing root information 
 *  with default root-id.
 *  Creates root kvnode and sets it.
 *
 *  @param[in] ns - namespace object for extracting index.
 *  @param[in] root_node_info - root attributes.
 *  @param[out] kvtree -Allocated kvtree with root.
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvtree_create(struct namespace *ns, struct kvnode_info *root_node_info,
                  struct kvtree **kvtree);

/** Deletes a kvtree.
 *  Deletes root_info associated with rood_id.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvtree_delete(struct kvtree *kvtree);

/* Initializes the kvtree with namespace obj, default root_id and 
 * based on namespace fid opens the index.
 *
 * @param[in] ns - namespace object
 * @param[out] kvtree - initialized kvtree
 * */
int kvtree_init(struct namespace *ns, struct kvtree *kvtree);

/* Closes the index of kvtree */
int kvtree_fini(struct kvtree *kvtree);

#endif /* _KVTREE_H_ */
