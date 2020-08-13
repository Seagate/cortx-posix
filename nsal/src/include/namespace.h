/*
 * Filename: namespace.h
 * Description: namespace - Data types and declarations for NSAL namespace.
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
 */

#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_

#include "str.h" /*str256_t*/
#include "kvstore.h" /*kvstore api*/

#define NS_ID_INIT 2  /* @todo change to 1 when module level intitalization is fixed */

/* Forward declaration */
struct namespace;
struct collection_item;

/******************************************************************************/
/* Methods */

/** Initialize namespace.
 *
 *  @param meta_index[in] - namespace meta index
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_module_init(struct kvs_idx *meta_index);

/** finalize namespace.
 *
 *  @param void.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_module_fini(void);

/** allocates namespace object.
 *
 *  @param name[in] - namespace name.
 *  @param ret_ns[out] - namespace object.
 *  @param ns_size[out] - namespace object size.
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_create(const str256_t *name, struct namespace **ret_ns, size_t *ns_size);

/** Deletes namespace object.
 *
 *  @param ns[in]: namespace object.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_delete(struct namespace *ns);

/** gives next namespace id.
 *
 *  @param ns_id[out]: next namespace object id.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_next_id(uint16_t *ns_id);

/** scans the namespace table.
 * Callback needs to copy the buffer containing ns, as it will be deleted in
 * the next iteration.
 * @param : ns_scan_cb, function to be executed for each found namespace.
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int ns_scan(void (*ns_scan_cb)(struct namespace *, size_t ns_size));

/** get namespace's name as str256_t obj.
 * For given str256_t obj, memory is not allocated by the caller,
 * namespace's buffer is assigned to str256_t obj.
 * Caller need-not free str256_t obj.
 * @param ns[in]: namespace obj.
 * @param name[out]: str256_t obj.
 *
 * @return void.
 */
void ns_get_name(struct namespace *ns, str256_t **name);

/** get namespace's index.
 *
 * namespace's buffer is assigned to index obj.
 * Caller need-not free index obj.
 * @param ns[in]: namespace obj.
 * @param ns_fid[out]: namespace's fid.
 *
 * @return void.
 */
void ns_get_fid(struct namespace *ns, kvs_idx_fid_t *ns_fid);

/** get namespace's id.
 *
 * @param ns[in]: namespace obj.
 * @param ns_id[out]: namespace's id.
 *
 * @return void.
 */
void ns_get_id(struct namespace *ns, uint16_t *ns_id);
#endif /* _NAMESPACE_H_ */
