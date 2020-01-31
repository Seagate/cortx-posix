/*
 * Filename:         kvstore.h
 * Description:      Key Value Store module of NSAL

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains KVStore framework infrastructure.
 There are currently 2 users of this infrastructure,
	1. eos_kvs
	2. redis

 This gives capability of basic key-value storing.
*/

#ifndef _KVSTORE_H
#define _KVSTORE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <utils.h>

#define KLEN (256)
#define VLEN (256)

struct kvstore_fid {
	uint64_t f_hi;
	uint64_t f_lo;
};

typedef int (*kvstore_alloc_t) (void **ptr, size_t size);
typedef void (*kvstore_free_t) (void *ptr);

int kvstore_alloc(void **ptr, uint64_t size);
void kvstore_free(void *ptr);

/* Forward declarations */
struct kvstore_ops;
struct kvstore_index_ops;
struct kvstore_kv_ops;
struct kvstore_index;
struct kvstore_iter;
struct kvstore_prefix_iter;

struct kvstore {
	/* Type of kvstore, current could be 2, {mero or redis} */
	char *type;
	/* Config for the kvstore specified type */
	struct collection_item *cfg;
	struct kvstore_ops *kvstore_ops;
	struct kvstore_index_ops *index_ops;
	struct kvstore_kv_ops *kv_ops;
	/* Not used currently */
	int flags;
};

/**
 * Return the global kvstore object.
 */
struct kvstore *kvstore_get(void);

struct kvstore_index_ops {
	int (*index_create) (struct kvstore *kvstore_obj,
			     const struct kvstore_fid *fid,
			     struct kvstore_index *index);
	int (*index_delete) (struct kvstore *kvstore_obj,
			     const struct kvstore_fid *fid);
	int (*index_open) (struct kvstore *kvstore_obj,
			   const struct kvstore_fid *fid,
			   struct kvstore_index *index);
	int (*index_close) (struct kvstore *kvstore_obj,
			    struct kvstore_index *index);
};

struct kvstore_ops {
	int (*init) (struct collection_item *cfg);
	int (*fini) (void);
	int (*alloc) (void **ptr, size_t size);
	void (*free) (void *ptr);
};

int kvstore_init(struct kvstore *kvstore_obj, char *type,
		 struct collection_item *cfg,
		 int flags, struct kvstore_ops *kvstore_ops,
		 struct kvstore_index_ops *index_ops,
		 struct kvstore_kv_ops *kv_ops);

int kvstore_fini(struct kvstore *kvstore_obj);

struct kvstore_index {
	struct kvstore *kvstore_obj;
	void *index_priv;
	struct kvstore_fid idx_fid;
};

struct kvstore_kv_ops {
	int (*begin_transaction) (struct kvstore_index *index);
	int (*end_transaction) (struct kvstore_index *index);
	int (*discard_transaction) (struct kvstore_index *index);
	int (*get_bin) (struct kvstore_index *index, void *k, const size_t klen,
			void **v, size_t *vlen);
	int (*get4_bin) (void *k, const size_t klen, void **v, size_t *vlen);
	int (*set_bin) (struct kvstore_index *index, void *k, const size_t klen,
			void *v, const size_t vlen);
	int (*set4_bin) (void *k, const size_t klen, void *v,
			 const size_t vlen);
	int (*del_bin) (struct kvstore_index *index, const void *k, size_t klen);
	bool (*kv_find) (struct kvstore_prefix_iter *iter);
	bool (*kv_next) (struct kvstore_prefix_iter *iter);
	void (*kv_fini) (struct kvstore_prefix_iter *iter);
	void (*kv_get) (struct kvstore_iter *iter, void **key, size_t *klen,
	                void **val, size_t *vlen);
};

/** Max size of implementation-defined data for a kvstore_iter. */
#define KVSTORE_ITER_PRIV_DATA_SIZE 128

/*
 * @todo: NSAL: MR: 1, comment, why separate,
 * kvstore_iter & kvstore_prefix_iter is needed?, investigation,
 * would be part of future work, when common
 * iterator objects would be implemented for mero &
 * redis.
 */
struct kvstore_iter {
	struct kvstore_index idx;
	int inner_rc;
	char priv[KVSTORE_ITER_PRIV_DATA_SIZE];
};

struct kvstore_prefix_iter {
	struct kvstore_iter base;
	const void *prefix;
	size_t prefix_len;
};

/** Batch Operation Implementation in kvstore */

struct kvpair {
	buff_t key, val;
};

struct kvgroup {
	/* Array of kv-pairs*/
	struct kvpair **kv_list;
	/* Maximum size of the kvpair list*/
	uint32_t kv_max;
	/* Number of kvpair present in group*/
	uint32_t kv_count;
};

/** Allocates memory for a new kvpair.
 *  @param kv - pointer of kvpair to which memory is allocated.
 */
int kvpair_alloc(struct kvpair **kv);

/** Frees memory for a given kvpair.
 *  @param kv - pointer to kvpair.
 */
void kvpair_free(struct kvpair *kv);

/** Initializes kvpair with given key and value.
 *  @param[in] kv - pointer to new kvpair
 *  @param[in] key - buffer of key
 *  @param[in] klen - length of key buffer
 *  @param[in] val - buffer of value
 *  @param[in] vlen - length of value buffer
 */
void kvpair_init(struct kvpair *kv, void *key, const size_t klen, void *val,
                 const size_t vlen);

/** Allocate array of kvpair with the given size inside kv_group.
 *  @param[in] kv_grp - pointer to kv_group object
 *  @param[in] size - length of kvpair array
 *  @return 0 if successful, else return error code.
 */
int kvgroup_init(struct kvgroup *kv_grp, const uint32_t size);

/** Add kvpair in kv_group.
 *  @param[in] kv_grp - pointer to kv_group
 *  @param[in] kv - kvpair to be added
 *  @return 0 if successful, else return error code.
 */
int kvgroup_add(struct kvgroup *kv_grp,
                struct kvpair *kv);

/** Free memory for array of kvpair in kv_group.
 *  @param[in] kv_grp - pointer to kv_group
 */
void kvgroup_fini(struct kvgroup *kv_grp);

/** Iterate over the values in kv group
 *  @param[in] kv_grp - pointer to kv_group, which is iterated
 *  @param[in] index - index of value to be returned
 *  @param[out] value - return value buffer
 *  @param[out] vlen - return value buffer length
 *  return 0 if value is present for a kvpair
 */
int kvgroup_kvpair_get(struct kvgroup *kv_grp, const int index,
                        void **value, size_t *vlen);

#endif
