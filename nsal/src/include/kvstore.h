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
	int (*index_global) (struct kvstore *kvstore,
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
};

/** Max size of implementation-defined data for a kvstore_iter. */
#define KVSTORE_ITER_PRIV_DATA_SIZE 64

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

/**
 * Find first record with the specified prefix.
 * @return True if the start record found.
 */
bool kvstore_prefix_iter_find(struct kvstore_prefix_iter *iter);

/**
 * Find the record following by the current record.
 * @return True if the next record found.
 */
bool kvstore_prefix_iter_next(struct kvstore_prefix_iter *iter);

/**
 * Free resources allocated by find/next calls.
 */
void kvstore_prefix_iter_fini(struct kvstore_prefix_iter *iter);

/**
 * Get pointer to key data.
 * @param[out] buf View of key data owned by iter.
 * @return Size of key.
 */
size_t kvstore_iter_get_key(struct kvstore_iter *iter, void **buf);

/**
 * Get pointer to value data.
 * @param[out] buf View of value data owner by iter.
 * @return Size of value.
 */
size_t kvstore_iter_get_value(struct kvstore_iter *iter, void **buf);

#endif
