/******************************************************************************/
/*
 * Filename: kvnode.h
 * Description: kvnode - Data types and declarations for kvnode - component of
 *                       kvtree.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 */
/******************************************************************************/

#ifndef _KVNODE_H_
#define _KVNODE_H_

/** # KVNode API overview
 *
 * The KVnode module API allows users to perform operations on kvnode which
 * is an in-memory representation of a node.
 *
 * kvnode_init - Constructor for initialising node without looking it up on disk.
 *               Use-case: Creating a new file/dir/symlink (non-existing file
 *               object).
 * kvnode_load - Constructor for initialising node using information stored on
 *               disk (de-serialize).
 *               This function requires node to be initialized and dumped to
 *               the disk.
 *               Use-case: 1. Reading the node present on disk.
 *                         2. Updating a node (basic attributes).
 * kvnode_dump - Dumps node on disk (serialize)
 * kvnode_delete - Removes a node the from disk
 * kvnode_fini - Destroys resources held by kvnode object created during
 *               init/load (destructor)
 *
 * # Memory
 *
 * KVnodes are initialized using either kvnode_init or kvnode_load constructors.
 * kvnode_fini is used to free the resources taken by a kvnode object.
 * The buffer passed to kvnode_init is copied into internal memory buffer
 * inside kvnode.
 *
 * # KVNode usage examples
 *
 * The following snippets describe how to use the KVNode API in the common
 * use-cases like initialize a node, delete it, update it.
 *
 * NOTE: 'data' used in below examples act as node's basic attributes.
 *       Its (void *), but can be of any data-type.
 *       For EFS File-systems, this data can be stats of a file.
 *
 * ## Initialize node
 *
 * To initialize a node from node id and attributes without looking it up on disk.
 *
 * @{code}
 * node_id = 1;
 * void *data = ...;
 * struct kvnode node; //Create a kvnode obj and pass it to init.
 * RC_WRAP(kvnode_init, tree, &node_id, data, sizeof(data), &node);
 * ... //further node operation
 * RC_WRAP(kvnode_fini, &node);
 * @{endcode}
 *
 * ## Write node to disk
 *
 * To init a node from node_id and attrs, then store it on disk.
 * We need to chain init and dump:
 *
 * @{code}
 * node_id = 1;
 * void *data = ...;
 * data_size = 10;
 * struct kvnode node;
 * RC_WRAP(kvnode_init, tree, &node_id, data, data_size, &node);
 * RC_WRAP(kvnode_dump, &node);
 * RC_WRAP(kvnode_fini, &node);
 * @{endcode}
 *
 * ## Delete node
 *
 * To delele a kvnode from the disk, we can combine it with init or with load.
 *
 * # Case 1: delete existing node (by id)
 *
 * @{code}
 * node_id = 1;
 * struct kvnode node;
 * RC_WRAP(kvnode_load, tree, &node_id, &node);
 * RC_WRAP(kvnode_delete, &node);
 * RC_WRAP(kvnode_fini, &node);
 * @{endcode}
 *
 * # Case 2: To avoid lookup and use node_id/node_attr directly.
 *
 * @{code}
 * node_id = 1;
 * void *data = ...;
 * struct kvnode node;
 * RC_WRAP(kvnode_init, tree, &node_id, data, sizeof(data), &node);
 * RC_WRAP(kvnode_delete, &node);
 * RC_WRAP(kvnode_fini, &node);
 * @{endcode}
 *
 * ## Update basic attributes
 *
 * To update the basic attributes of a node, we have to combine load with dump.
 *
 * @{code}
 * node_id = 1;
 * struct kvnode node;
 * RC_WRAP(kvnode_load, tree, &node_id, &node);
 * // Modify contents of node->basic_attr
 * RC_WRAP(kvnode_dump, &node); //write back updated node to disk
 * RC_WRAP(kvnode_fini, &node);
 * @{endcode}
 */

struct kvtree;

/* Kvnode is in-memory representation of a node that is linked
 * (or to be linked) into a kvtree.
 * Each kvnode always have the following information:
 *  - node ID (node id).
 *  - basic attribute (node basic attr).
 *   The basic attribute is something that represents the very
 *   core of this node. For example, in POSIX FS it will be
 *   struct stat because each FS node always has it.
 */
struct kvnode {
	struct kvtree *tree;         /* kvtree pointer to which the node belongs */
	node_id_t node_id;             /* 128-bit Identifier for every node */
	struct kvnode_basic_attr *basic_attr; /* pointer to node's basic attributes */
};

/* On-disk structure for storing node basic attributes.
 * Uses cases: EFS file stats */
struct kvnode_basic_attr;

#define NODE_ID_F "{hi=%" PRIu64 ", lo=%" PRIu64 "}"
#define NODE_ID_P(_nid) (_nid)->f_hi, (_nid)->f_lo

#define KVNODE_NULL_ID (node_id_t) { .f_hi = 0, .f_lo = 0 }

/** Initializes a kvnode with node basic attributes for a given id(128 bit) and
 *  attributes.
 *  Note: This function initialzes a kvnode without looking it up on the disk.
 *        It requires caller to pass a allocated kvnode object.
 *        Also it allocates memory seperately for internal kvnode_basic_attrs
 *        and contents are copied into it from buffer passed to kvnode_init.
 *
 *  @param[in] node_id - pointer to 128 bit node identifier
 *  @param[in] attr - buffer for basic attributes of the node
 *  @param[in] attr_size - size of basic attributes of node
 *  @param[out] node - kvnode initialized
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_init(struct kvtree *tree, const node_id_t *node_id,
                const void *attr, const size_t attr_size, struct kvnode *node);

/* Releases the resources held by kvnode
 *
 * @param[in] node - kvnode's basic attributes held by kvnode are released.
 */
void kvnode_fini(struct kvnode *node);

/** Stores node basic attributes on disk.
 *
 *  @param[in] node - kvnode to be stored.
 *  NOTE: Node should be filled with valid kvtree, node_id and basic attributes.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_dump(const struct kvnode *node);

/** Loads node basic attribute from the disk.
 *  It is another way of initialising a kvnode using information stored on the
 *  disk.
 *
 *  @param[in] tree - kvtree in which the node is present
 *  @param[in] node_id - node ID whose basic attributes are to be loaded.
 *  @param[out] node - kvnode initialized with tree, node_id and
 *                     node basic attr.
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_load(struct kvtree *tree, const node_id_t *node_id,
                struct kvnode *node);

/* Deletes kvnode from the disk.
 *
 *  @param[in] node - node to be deleted.
 *  NOTE: Node should be initialized with tree and node_id atleast.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_delete(const struct kvnode *node);

/* Stores system attributes (based on numeric id) of a node on the disk.
 * NOTE: The node should be initialized with valid tree, node_id atleast.
 *
 * @param[in] node - node whose system attrs are to be stored.
 * @param[in] key - numeric id which represents the type of system attr.
 * @param[in] value - buffer for contents of system attr
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_set_sys_attr(const struct kvnode *node, const int key,
                        const buff_t value);

/* Fetches specific type of system attributes (based on numeric id) of a node
 * from the disk by the given node_id.
 *
 * @param[in] node - node whose system attrs are to be fetched.
 * @param[in] key - numeric id which represents the type of system attr.
 * @param[out] value - buffer where contents of system attr and length are
 *                     fetched
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_get_sys_attr(const struct kvnode *node, const int key, buff_t *value);

/* Deletes specific type of system attributes (based on numeric id) of a node
 * from the disk.
 * NOTE: The node should be initialized with valid tree, node_id atleast.
 *
 * @param[in] node - node whose system attrs are to be deleted.
 * @param[in] key - numeric id which represents the type of system attr.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_del_sys_attr(const struct kvnode *node, const int key);

#endif /* _KVNODE_H_ */
