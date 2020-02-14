/*
 * Filename:         eos_kvstore.h
 * Description:      Provides EOS specific implementation of
 * 		     KVStore module of NSAL.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This EOS specific implementation is based on m0_clovis's index entity.
*/

#ifndef _EOS_KVSTORE_H
#define _EOS_KVSTORE_H

#include "kvstore.h"

extern struct kvstore_ops eos_kvs_ops;

int eos_kvs_alloc(void **ptr, size_t size);
void eos_kvs_free(void *ptr);

const char *eos_kvs_get_gfid(void);

int eos_kvs_fid_from_str(const char *fid_str, kvs_fid_t *out_fid);

int eos_kvs_get_list_size(void *ctx, char *pattern, size_t plen);

/** Set list of key-value pairs
 *  @param[in] kv_grp - pointer to kv_group
 *  @return 0 if successful, else return error code.
 */
int eos_kvs_list_set(struct kvs_idx *index,
                     const struct kvgroup *kv_grp);

/** Get list of key-value pairs
 *  @param kv_grp - pointer to kv_group which fetches values for given keys
 *  @return 0 if successful, else return error code.
 */
int eos_kvs_list_get(struct kvs_idx *index,
                     struct kvgroup *kv_grp);

#endif
