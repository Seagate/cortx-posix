/*
 * Filename: tenant.h
 * Description: tenant - Data types and declarations for NSAL tenant.
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

#ifndef _TENANT_H_
#define _TENANT_H_

#include "str.h" /*str256_t*/
#include "kvstore.h" /*kvstore api*/

#define TENANT_ID_INIT 2  /* @todo change to 1 when module level intitalization is fixed */

/* Forward declaration */
struct tenant;

/******************************************************************************/
/* Methods */

/** Initialize tenant.
 *
 *  @param ns_meta_index[in]: namespace meta index.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_module_init(struct kvs_idx *ns_meta_index);

/** finalize tenant.
 *
 *  @param void.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_module_fini(void);

/** allocates tenant objecti and store options in kvs.
 *
 *  @param name[in] - tenant name.
 *  @param ret_tenant[out] - tenant object.
 *  @param ns_id[in] - Id of namespace object.
 *  @param options[in] - tenant object options.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_create(const str256_t *name, struct tenant **ret_tenant, uint16_t ns_id,
		  const char *options);

/** Deletes tenant object.
 *
 *  @param tenant[in]: tenant object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_delete(struct tenant *tenant);

/** gives next tenant id.
 *
 *  @param tenant_id[out]: next tenant object id.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_next_id(uint16_t *tenant_id);

/** scans the tenant table.
 *  Iteration stops if "tenant_scan_cb" returns non-zero return code.
 *  The user is responsible for preserving this ret code inside the context
 *  because tenant_scan simply stops execution and returns 0 in case if
 *  "tenant_scan_cb" returns non-zero code.
 *
 *  @param : tenant_scan_cb, function to be executed for each found tenant.
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_scan(int (*tenant_scan_cb)(void *cb_ctx, struct tenant *tenant),
		void *cb_ctx);

/** get tenant's name as str256_t obj.
 * For given str256_t obj, memory is not allocated by the caller,
 * tenant's buffer is assigned to str256_t obj.
 * Caller need-not free str256_t obj.
 * @param tenant[in]: tenant obj.
 * @param name[out]: str256_t obj.
 *
 * @return void.
 */
void tenant_get_name(struct tenant *tenant, str256_t **name);

/**
 * getter function for tenant info.
 *
 * @param tenant[in]: tenant object.
 * @param info[out]: information of the tenant object
 *
 * @return void.
 */
void tenant_get_info(struct tenant *tenant, void **info);

/** Copy tenant obj into dst..
 *
 *  @param src[in]: source
 *  @param dest[out]: destination.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int tenant_copy(const struct tenant *src, struct tenant **dst);

/** Free tenaant object.
 *
 *  @param: tenant obj.
 *
 *  @return void.
 */
void tenant_free(struct tenant *obj);
#endif /* _TENANT_H_ */
