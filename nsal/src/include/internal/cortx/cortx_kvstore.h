/*
 * Filename:         cortx_kvstore.h
 * Description:      Provides CORTX specific implementation of
 * 		     KVStore module of NSAL.
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

/*
 * This CORTX specific implementation is based on m0_clovis's index entity.
 */

#ifndef _CORTX_KVSTORE_H
#define _CORTX_KVSTORE_H

#include "kvstore.h"

extern struct kvstore_ops cortx_kvs_ops;

int cortx_kvs_alloc(void **ptr, size_t size);
void cortx_kvs_free(void *ptr);

const char *cortx_kvs_get_gfid(void);

int cortx_kvs_fid_from_str(const char *fid_str, kvs_idx_fid_t *out_fid);

int cortx_kvs_get_list_size(void *ctx, char *pattern, size_t plen);

/** Set list of key-value pairs
 *  @param[in] kv_grp - pointer to kv_group
 *  @return 0 if successful, else return error code.
 */
int cortx_kvs_list_set(struct kvs_idx *index,
                     const struct kvgroup *kv_grp);

/** Get list of key-value pairs
 *  @param kv_grp - pointer to kv_group which fetches values for given keys
 *  @return 0 if successful, else return error code.
 */
int cortx_kvs_list_get(struct kvs_idx *index,
                     struct kvgroup *kv_grp);

#endif
