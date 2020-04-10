/*
 * Filename: nsal.h
 * Description: Data types and declarations for NSAL.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 */

#ifndef _NSAL_H_
#define _NSAL_H_

typedef obj_id_t node_id_t; /* Act as node identifier in kvtree */

typedef enum kvtree_key_type {
	/* KVTree key types associated with particular version of kvtree. */
	KVTREE_KEY_TYPE_NODE_INFO = 20,
	KVTREE_KEY_TYPE_CHILD,
	KVTREE_KEY_TYPE_LINK,
	KVTREE_KEY_TYPE_XATTR,
	KVTREE_KEY_TYPE_INVALID,
	KVTREE_KEY_TYPES = KVTREE_KEY_TYPE_INVALID,

} kvtree_key_type_t;


/******************************************************************************/
/* Methods */

/** Initialize nsal.
 *
 *  @param "struct collection_item *cfg_items". 
 *   
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int nsal_init(struct collection_item *cfg_items);

/** finalize nsal.
 *  
 *  @param void.
 *  
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int nsal_fini(void);

#endif /* _NSAL_H_ */
