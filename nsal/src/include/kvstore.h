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
#include <str.h>
#include <ini_config.h>
#include "object.h"

#define KLEN (256)
#define VLEN (256)

typedef obj_id_t kvs_fid_t;

/* Forward declarations */
struct kvstore_ops;
struct kvs_idx;
struct kvs_itr;

/* Key prefix */
struct key_prefix {
        uint8_t k_type;
        uint8_t k_version;
} __attribute__((packed));

struct kvstore {
	/* Type of kvstore, current could be 2, {eos or redis} */
	char *type;
	struct kvstore_ops *kvstore_ops;
};

/**
 * Return the global kvstore object.
 */
struct kvstore *kvstore_get(void);

/* TODO: Can be moved to utils*/
int kvs_fid_from_str(const char *fid_str, kvs_fid_t *out_fid);

int kvs_alloc(struct kvstore *kvstore, void **ptr, size_t size);
void kvs_free(struct kvstore *kvstore, void *ptr);

int kvs_begin_transaction(struct kvstore *kvstore, struct kvs_idx *index);
int kvs_end_transaction(struct kvstore *kvstore, struct kvs_idx *index);
int kvs_discard_transaction(struct kvstore *kvstore, struct kvs_idx *index);

int kvs_index_create(struct kvstore *kvstore, const kvs_fid_t *fid,
                     struct kvs_idx *index);
int kvs_index_delete(struct kvstore *kvstore, const kvs_fid_t *fid);
int kvs_index_open(struct kvstore *kvstore, const kvs_fid_t *fid,
                   struct kvs_idx *index);
int kvs_index_close(struct kvstore *kvstore, struct kvs_idx *index);

/* Key-Value operations */
int kvs_get(struct kvstore *kvstore, struct kvs_idx *index, void *k, const size_t klen,
            void **v, size_t *vlen);
int kvs_set(struct kvstore *kvstore, struct kvs_idx *index, void *k, const size_t klen,
            void *v, const size_t vlen);
int kvs_del(struct kvstore *kvstore, struct kvs_idx *index, const void *k, size_t klen);

/* Key-Value iterator API
 *
 * Usage: For using iterator:-
 * 1. kvs_itr_find - For a given prefix and prefix len, allocates memory and
 *                   initializes iterator with prefix and len.
 *                   Scans key-value and checks the fetched key with the prefix,
 *                   stores in a buffer.
 *                   Returns 0 if successful, else returns -ENOENT if match
 *                   not found.
 *                   For failure of kvs_itr_find, returns err-code
 *                   (other than -ENOENT).
 *
 * 2. kvs_itr_get -  For a given key and value pointer, kvs_itr_get assigns the
 *                   fetched key and value from the previous scan (buffer) to
 *                   these pointers.
 *
 * 3. kvs_itr_next - For a given iterator (which is initialized by kvs_itr_find),
 *                   fetches the next key-value.
 *                   Frees previous value buffer before scanning next key-value.
 *                   Stores new key-value to the buffer.
 *
 * 4. kvs_itr_fini - For a given iterator, frees the buffer for key-value.
 *                   Frees the iterator.
 */

int kvs_itr_find(struct kvstore *kvstore, struct kvs_idx *index, void *prefix,
                  const size_t prefix_len, struct kvs_itr **iter);
int kvs_itr_next(struct kvstore *kvstore, struct kvs_itr *iter);
void kvs_itr_fini(struct kvstore *kvstore, struct kvs_itr *iter);
void kvs_itr_get(struct kvstore *kvstore, struct kvs_itr *iter, void **key, size_t *klen,
                 void **val, size_t *vlen);

struct kvstore_ops {
	/* Basic constructor and destructor */
	int (*init) (struct collection_item *cfg);
	int (*fini) (void);
	int (*alloc) (void **ptr, size_t size);
	void (*free) (void *ptr);

	/* Transaction related operations */
	int (*begin_transaction) (struct kvs_idx *index);
	int (*end_transaction) (struct kvs_idx *index);
	int (*discard_transaction) (struct kvs_idx *index);

	/* Index operations */
	int (*index_create) (const kvs_fid_t *fid, struct kvs_idx *index);
	int (*index_delete) (const kvs_fid_t *fid);
	int (*index_open) (const kvs_fid_t *fid, struct kvs_idx *index);
	int (*index_close) (struct kvs_idx *index);

	/* Key-Value operations */
	int (*get_bin) (struct kvs_idx *index, void *k, const size_t klen,
	                void **v, size_t *vlen);
	int (*get4_bin) (void *k, const size_t klen, void **v, size_t *vlen);
		 /* TODO: Remove get4_bin interface */
	int (*set_bin) (struct kvs_idx *index, void *k, const size_t klen,
	                void *v, const size_t vlen);
	int (*set4_bin) (void *k, const size_t klen, void *v,
	                 const size_t vlen); /* TODO: Remove set4_bin interface */
	int (*del_bin) (struct kvs_idx *index, const void *k, size_t klen);

	/* Key-Value search */
	int (*kv_find) (struct kvs_itr *iter);
	int (*kv_next) (struct kvs_itr *iter);
	void (*kv_fini) (struct kvs_itr *iter);
	void (*kv_get) (struct kvs_itr *iter, void **key, size_t *klen,
	                void **val, size_t *vlen);
};

int kvs_init(struct kvstore *kvstore, struct collection_item *cfg);

int kvs_fini(struct kvstore *kvstore);

struct kvs_idx {
	kvs_fid_t index_fid;
	void *index_priv;
};

/** Max size of implementation-defined data for a kvs_itr. */
#define KVSTORE_ITER_PRIV_DATA_SIZE 128

struct kvs_itr {
	struct kvs_idx idx;
	buff_t prefix;
	int inner_rc;
	char priv[KVSTORE_ITER_PRIV_DATA_SIZE];
};

/** Batch Operation Implementation in kvstore */

struct kvpair {
	buff_t key;
	buff_t val;
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
