/*
 * Filename:         eos_kvstore.c
 * Description:      Implementation of EOS KVStore.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains APIs which implement NSAL's KVStore framework,
 on top of eos clovis index APIs.
*/
#include "eos/eos_kvstore.h"

int eos_kvs_init(struct collection_item *cfg_items)
{
	return m0init(cfg_items);
}

int eos_kvs_fini(void)
{
	m0fini();
	return 0;
}

int eos_kvs_alloc(void **ptr, size_t size)
{
	*ptr = m0kvs_alloc(size);
	if (*ptr == NULL)
		return -ENOMEM;

	return 0;
}

void eos_kvs_free(void *ptr)
{
	return m0kvs_free(ptr);
}

struct kvstore_ops eos_kvs_ops = {
	.init = eos_kvs_init,
	.fini = eos_kvs_fini,
	.alloc = eos_kvs_alloc,
	.free = eos_kvs_free
};


int eos_kvs_index_create(struct kvstore *kvstore, const struct kvstore_fid *fid,
		      struct kvstore_index *index)
{
        int rc;
	struct m0_uint128 mfid = M0_UINT128(0, 0);
	struct m0_clovis_idx *idx = NULL;

	index->kvstore_obj = kvstore;
	index->index_priv = NULL;
	index->idx_fid = *fid;

	mfid.u_hi = fid->f_hi;
	mfid.u_lo = fid->f_lo;

        rc = m0idx_create(&mfid, &idx);
        if (rc != 0) {
                fprintf(stderr, "Failed to create index, fid=%" PRIx64 ":%" PRIx64 "", 
			mfid.u_hi, mfid.u_lo);
		goto out;
	}

	/** Use KVStore index's priv to track Mero clovis index */
	index->index_priv = idx;
out:
        return rc;
}

int eos_kvs_index_delete(struct kvstore *kvstore_obj,
		      const struct kvstore_fid *fid)
{
        int rc;
	struct m0_uint128 mfid = M0_UINT128(0, 0);

	mfid.u_hi = fid->f_hi;
	mfid.u_lo = fid->f_lo;

        rc = m0idx_delete(&mfid);
        if (rc != 0) {
                fprintf(stderr, "Failed to delete index, fid=%" PRIx64 ":%" PRIx64 "", 
			mfid.u_hi, mfid.u_lo);
	}

        return rc;
}

int eos_kvs_index_open(struct kvstore *kvstore, const struct kvstore_fid *fid,
		     struct kvstore_index *index)
{
	int rc;
	struct m0_uint128 mfid = M0_UINT128(0, 0);
	struct m0_clovis_idx *idx = NULL;

	index->kvstore_obj = kvstore;
	index->index_priv = NULL;
	index->idx_fid = *fid;;
	
	mfid.u_hi = fid->f_hi;
	mfid.u_lo = fid->f_lo;

	rc = m0idx_open(&mfid, &idx);
	if (rc != 0) {
		fprintf(stderr, "Failed to open index, fid=%" PRIx64 ":%" PRIx64 "", 
			mfid.u_hi, mfid.u_lo);
		goto out;
	}

	index->index_priv = idx;
out:
	return rc;
}

int eos_kvs_index_close(struct kvstore *kvstore, struct kvstore_index *index)
{
	struct m0_clovis_idx *idx = (struct m0_clovis_idx *)index->index_priv;

        m0idx_close(idx);

	index->kvstore_obj = NULL;
	index->index_priv = NULL;
	index->idx_fid.f_hi = 0;
	index->idx_fid.f_lo = 0;

	return 0;
}

struct kvstore_index_ops eos_kvs_index_ops = {
	.index_create = eos_kvs_index_create,
	.index_delete = eos_kvs_index_delete,
	.index_open = eos_kvs_index_open,
	.index_close = eos_kvs_index_close
};

int eos_kvs_begin_transaction(struct kvstore_index *index)
{
	return 0;
}

int eos_kvs_end_transaction(struct kvstore_index *index)
{
	return 0;
}

int eos_kvs_discard_transaction(struct kvstore_index *index)
{
	return 0;
}

int eos_kvs_get_bin(struct kvstore_index *index, void *k, const size_t klen,
		 void **v, size_t *vlen)
{
	return m0kvs_get(index->index_priv, k, klen, v, vlen);
}

int eos_kvs4_get_bin(void *k, const size_t klen, void **v, size_t *vlen)
{
	return m0kvs4_get(k, klen, v, vlen);
}

int eos_kvs_set_bin(struct kvstore_index *index, void *k, const size_t klen,
		 void *v, const size_t vlen)
{
	return m0kvs_set(index->index_priv, k, klen, v, vlen);
}

int eos_kvs4_set_bin(void *k, const size_t klen, void *v, const size_t vlen)
{
	return m0kvs4_set(k, klen, v, vlen);
}

int eos_kvs_del_bin(struct kvstore_index *index, const void *key, size_t klen)
{
	return m0kvs_del(index->index_priv, (void *) key, klen);
}

struct kvstore_kv_ops eos_kvs_kv_ops = {
	.begin_transaction = eos_kvs_begin_transaction,
	.end_transaction = eos_kvs_end_transaction,
	.discard_transaction = eos_kvs_discard_transaction,
	.get_bin = eos_kvs_get_bin,
	.get4_bin = eos_kvs4_get_bin,
	.set_bin = eos_kvs_set_bin,
	.set4_bin = eos_kvs4_set_bin,
	.del_bin = eos_kvs_del_bin
};

char *eos_kvs_get_gfid(void)
{
	return m0_get_gfid();
}

int eos_kvs_fid_from_str(const char *fid_str, struct kvstore_fid *out_fid)
{
	return m0_fid_sscanf(fid_str, (struct m0_fid *) out_fid);
}

bool get_list_cb_size(char *k, void *arg)
{
	int size;

	memcpy((char *)&size, (char *)arg, sizeof(int));
	size += 1;
	memcpy((char *)arg, (char *)&size, sizeof(int));

	return true;
}

int eos_kvs_get_list_size(void *ctx, char *pattern, size_t plen)
{
	char initk[KLEN];
	int size = 0;
	int rc;

	strcpy(initk, pattern);
	initk[plen - 2] = '\0';

	rc = m0_pattern_kvs(ctx, initk, pattern,
			     get_list_cb_size, &size);
	if (rc < 0)
		return rc;

	return size;
}

bool eos_kvs_prefix_iter_has_prefix(struct kvstore_prefix_iter *iter)
{
	void *key;
	size_t key_len;

	key_len = eos_kvs_iter_get_key(&iter->base, &key);
	assert(key_len >= iter->prefix_len);
	return memcmp(iter->prefix, key, iter->prefix_len) == 0;
}

bool eos_kvs_prefix_iter_find(struct kvstore_prefix_iter *iter)
{
	return m0_key_iter_find(&iter->base, iter->prefix, iter->prefix_len) &&
		eos_kvs_prefix_iter_has_prefix(iter);
}

bool eos_kvs_prefix_iter_next(struct kvstore_prefix_iter *iter)
{
	return m0_key_iter_next(&iter->base) &&
		eos_kvs_prefix_iter_has_prefix(iter);
}

void eos_kvs_prefix_iter_fini(struct kvstore_prefix_iter *iter)
{
	m0_key_iter_fini(&iter->base);
}

size_t eos_kvs_iter_get_key(struct kvstore_iter *iter, void **buf)
{
	return m0_key_iter_get_key(iter, buf);
}

size_t eos_kvs_iter_get_value(struct kvstore_iter *iter, void **buf)
{
	return m0_key_iter_get_value(iter, buf);
}

