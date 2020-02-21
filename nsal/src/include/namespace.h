/*
 * Filename: namespace.h
 * Description: namespace - Data types and declarations for NSAL namespace.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 */

#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_

#include <str.h> /*str256_t*/
#include <kvstore.h> /*kvstore api*/

#define NS_ID_INIT 2  /* @todo change to 1 when module level intitalization is fixed */

/* Forward declaration */
struct namespace;
struct ns_itr;

/******************************************************************************/
/* Methods */

/** Initialize namespace.
 *
 *  @param cfg[in] - collection item.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_init(struct collection_item *cfg);

/** finalize namespace.
 *
 *  @param void.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_fini(void);

/** allocates namespace object.
 *
 *  @param name[in] - namespace name.
 *  @param ret_ns[out] - namespace object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_create(str256_t *name, struct namespace **ret_ns);

/** Deletes namespace object.
 *
 *  @param ns[in]: namespace object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_delete(struct namespace *ns);

/** gives next namespace id.
 *
 *  @param nsobj_id[out]: next namespace object id.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_next_id(uint32_t *nsobj_id);
	
/** scans the namespace table. 
 *
 * @param iter[out]: opaque iter for upper layer.
 * @param ns[out]: namespace object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */ 
int ns_scan(void (*cb)(struct namespace *));
/** get namespace's name as str256_t obj.
 *
 * @param ns[in]: namespace obj.
 * @param out[out]: str256_t obj.
 *
 * @return void.
 */
void get_ns_name(struct namespace *ns, str256_t **out);
#endif /* _NAMESPACE_H_ */
