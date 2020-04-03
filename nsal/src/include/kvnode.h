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

#ifndef _KVNODE_H_
#define _KVNODE_H_

struct kvtree;

struct kvnode {
	struct kvtree *kvtree;         /* kvtree pointer to which the node belongs */
	node_id_t node_id;             /* 128-bit Identifier for every node */
	struct kvnode_info *node_info; /* pointer to node attributes */
};

/* On-disk structure for storing node information.
 * Uses cases: EFS stats */
struct kvnode_info {
	uint16_t size;          /* size of node information buffer */
	char info[0];                /* node information buffer */
};

/** Creates a kvnode for a given id(128 bit) and node information.
 *  Stores node_info for that node id.
 *
 *  @param[in] node_id - 128 bit node identifier
 *  @param[in] node_info - kvnode buffer filled with node attributes
 *  @param[out] node - creates kvnode
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_create(struct kvtree *kvtree, node_id_t node_id,
                  struct kvnode_info *node_info, struct kvnode *node);

/** Allocates memory for kvnode_info and initializes node information buffer 
 *  into kvnode_info required for creating a kvnode.
 *
 *  @param[in] buff - node information buffer
 *  @param[in] size - buffer size
 *  @param[out] node_info - Allocated and initialized kvnode_info.
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_info_alloc(const void *buff, const size_t size,
                     struct kvnode_info **node_info);

/** Frees kvnode_info pointer used for node info .
 *
 *  @param[in] node_info - pointer to kvnode_info.
 */
void kvnode_info_free(struct kvnode_info *node_info);

/** Stores node info for a  kvnode.
 *
 *  @param[in] node - kvnode to be stored.
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_info_write(struct kvnode *node);

/** Fetches node info for a  kvnode.
 *
 *  @param[in] node - kvnode with node_id set.
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_info_read(struct kvnode *node);

/** Deletes/removes node info for a node_id.
 *
 *  @param[in] kvtree - pointer to kvtree.
 *  @param[in] node_id - node identifier which deletes the particular node_info
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_info_remove(struct kvtree *kvtree, node_id_t *node_id);

/* Deletes kvnode for a given node_id
 * Calls internally kvnode_del_info.
 *
 *  @param[in] kvtree - pointer to kvtree.
 *  @param[in] node_id - node identifier 
 *  @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvnode_delete(struct kvtree *kvtree, node_id_t *node_id);

#endif /* _KVNODE_H_ */
