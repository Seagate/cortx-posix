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
	KVTREE_KEY_TYPE_CHILD = 20,
	KVTREE_KEY_TYPE_BASIC_ATTR,
	KVTREE_KEY_TYPE_SYSTEM_ATTR,
	/* TODO: This enum entry should be removed because xattr is
	 * independent of kvtree. Right now we need it here to keep
	 * backward compatibility with kvtree key types and avoid conflict
	 * with xattr keys as both are stored in same index.
	 * When seperate xattr key type is added, this entry can be safely
	 * removed.
	 **/
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
