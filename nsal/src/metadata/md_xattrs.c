/*
 * Filename: md_xattrs.c
 * Description: Metadata - Implementation of Extended Attributes
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Subhash Arya <subhash.arya@seagate.com>
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <md_xattr.h>
#include <dirent.h>
#include <eos/eos_kvstore.h>
#include <common/log.h>


/* */
struct md_xattr_key {
	obj_id_t  xk_oid;
	md_key_md_t xk_md;
	md_str_t xk_name;
} __attribute((packed));

#define XATTR_KEY_PREFIX_INIT(__oid)		\
{						\
	.xk_oid	 = *__oid,			\
	.xk_md = {				\
		.k_type = MD_KEY_TYPE_XATTR,	\
		.k_version = MD_VERSION_0,	\
	},				        \
	.xk_name = {},				\
}

/** Pattern size of a xattr key, i.e. the size of xattr prefix. */
static const size_t md_xattr_key_psize =
	sizeof(struct md_xattr_key) - sizeof(md_str_t);

/** Dynamic size of a xattr name object. */
static inline size_t md_xattr_name_size(const md_str_t *name)
{
	/* sizeof (len field) + actual len + size of null-terminator */
	size_t result = sizeof(name->len) + name->len + 1;
	/* Dynamic size cannot be more than the max size of the structure */
	MD_DASSERT(result <= sizeof(*name));
	return result;
}

/**
 * Dynamic size of a xattr key, i.e. the amount of bytes to be stored in
 * the KVS storage.
 */
static inline size_t md_xattr_key_dsize(const struct md_xattr_key *key) {
	return md_xattr_key_psize + md_xattr_name_size(&key->xk_name);
}

static int md_xattr_alloc_init_key(const obj_id_t *oid, const char *name,
				   struct md_xattr_key **key)
{
	int rc;
	uint8_t len;
	struct kvstore *kvstor = kvstore_get();

	MD_DASSERT(kvstor != NULL);
	len = strlen(name);
	if (len > NAME_MAX) {
		rc = -ENAMETOOLONG;
		goto out;
	}

	MD_RC_WRAP_LABEL(rc, out, kvstor->kvstore_ops->alloc, (void **)key,
		         sizeof (struct md_xattr_key));

	(*key)->xk_oid = *oid;
	(*key)->xk_md.k_type = MD_KEY_TYPE_XATTR;
	(*key)->xk_md.k_version = MD_VERSION_0;
	(*key)->xk_name.len = len;
	memcpy((*key)->xk_name.str, name, (*key)->xk_name.len);
out:
	return rc;
}

int md_xattr_set(struct kvs_idx *idx, const obj_id_t *oid,
		 const char *name, const void *value, size_t size)
{
	int rc;
	struct md_xattr_key *key;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	MD_DASSERT(kvstor != NULL);
	MD_DASSERT(oid && name && value);
	MD_DASSERT(size != 0);

	index.index_priv = idx;

	if (size > MD_XATTR_SIZE_MAX) {
		rc = -ERANGE;
		goto out;
	}

	MD_RC_WRAP_LABEL(rc, out, md_xattr_alloc_init_key, oid, name, &key);

	MD_RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &index, key,
		         md_xattr_key_dsize(key), (void *)value, size);

free_key:
	kvs_free(kvstor, key);

out:
	log_trace("idx=%p oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s val=%p"
		  "size=%lu", idx, oid->f_hi, oid->f_lo, rc, name, value,
		  size);
	return rc;
}

int md_xattr_get(struct kvs_idx *idx, const obj_id_t *oid,
		 const char *name, void **value, size_t *size)
{
	int rc;
	struct md_xattr_key *key;
	void *read_val = NULL;
	size_t size_val = 0;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	MD_DASSERT(kvstor != NULL);
	MD_DASSERT(oid && name && size);

	index.index_priv = idx;

	MD_RC_WRAP_LABEL(rc, out, md_xattr_alloc_init_key, oid, name,  &key);

	MD_RC_WRAP_LABEL(rc, free_key, kvs_get, kvstor, &index, key,
		         md_xattr_key_dsize(key), &read_val, &size_val);

	*value = read_val;
	*size = size_val;

free_key:
	kvs_free(kvstor, key);

out:
	log_trace("idx=%p oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s val=%p "
		  "*size=%lu", idx, oid->f_hi, oid->f_lo, rc, name,
		  read_val, *size);
	return rc;
}

int md_xattr_exists(struct kvs_idx *idx, const obj_id_t *oid,
                    const char *name, bool *result)
{
	int rc;
	size_t size = 0;
	void  *val = NULL;

	MD_DASSERT(oid && name && result);

	rc = md_xattr_get(idx, oid, name, &val, &size);
	if ((rc == -ENOENT) || (rc == 0)) {
		*result = rc == 0;
		rc = 0;
	}
	else {
		log_err("md_xattr_get failed, rc=%d", rc);
	}

	log_trace("idx=%p oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s"
		  " size=%lu", idx, oid->f_hi, oid->f_lo, rc, name,
		   size);

	if (val != NULL) {
		md_free(val);
	}
	return rc;
}

int md_xattr_delete(struct kvs_idx *idx, const obj_id_t *oid,
		    const char *name)
{
	int rc;
	struct md_xattr_key *key;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	MD_DASSERT(kvstor != NULL);
	MD_DASSERT(oid && name);

	index.index_priv = idx;

	MD_RC_WRAP_LABEL(rc, out, md_xattr_alloc_init_key, oid, name,  &key);

	MD_RC_WRAP_LABEL(rc, free_key, kvs_del, kvstor, &index, key,
		         md_xattr_key_dsize(key));

free_key:
	kvs_free(kvstor, key);

out:
	log_trace("idx=%p oid=%" PRIx64 ":%" PRIx64 " rc=%d name=%s",idx,
		  oid->f_hi, oid->f_lo, rc, name);
	return rc;
}

int md_xattr_list(struct kvs_idx *idx, const obj_id_t *oid, void *buf,
		  size_t *count, size_t *size)
{
	int rc;
	struct kvstore *kvstor = kvstore_get();
	size_t klen, vlen;
	bool has_next = true;
	struct md_xattr_key prefix = XATTR_KEY_PREFIX_INIT(oid);
	const struct md_xattr_key *key = NULL;
	struct kvs_idx index;
	struct kvs_itr *iter = NULL;
	void *value;

	MD_DASSERT(kvstor != NULL);
	MD_DASSERT(buf != NULL);
	MD_DASSERT(size != NULL);
	MD_DASSERT(count != NULL);

	char *names = (char *)buf;
	*count = 0;
	size_t offset=0;

	if (*size > MD_XATTR_SIZE_MAX) {
		rc = -E2BIG;
		log_debug("Size too big! idx=%p, oid=%" PRIx64 ":%" PRIx64 ""
			  "size=%zu, rc=%d", idx, oid->f_hi, oid->f_lo, *size,
			  rc);
		goto err;
	}

	index.index_priv = idx;
	rc = kvs_itr_find(kvstor, &index, &prefix, md_xattr_key_psize, &iter);
	if (rc) {
		goto out;
	}
	while (has_next) {
		kvs_itr_get(kvstor, iter, (void **) &key, &klen, (void **) &value, &vlen);
		MD_DASSERT(key->xk_name.len != 0);
		MD_DASSERT(klen > md_xattr_key_psize);
		log_debug("xattr name=%s, len= %" PRIu8 "", key->xk_name.str,
			  key->xk_name.len);

		/* Account for `/0` while calculating offset and len. */
		offset += key->xk_name.len +  1;
		if ((offset > *size) && (*size != 0)) {
			rc =  -ERANGE;
			log_debug("Buffer too small, size=%zu, rc=%d",
				  *size, rc);
			goto out;
		}

		/* Do not memcpy when the caller is interested in knowing the
		   buffer size. */
		if (*size != 0) {
			memcpy(names, key->xk_name.str, key->xk_name.len + 1);
		}
		names += key->xk_name.len + 1;
		(*count)++;
		rc = kvs_itr_next(kvstor, iter);
		has_next = (rc == 0);

		log_debug("offset=%zu, has_next=%d, iter rc =%d), count=%zu",
			  offset, (int) has_next, rc, *count);
	}

out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	if (*size == 0) {
		rc = -ERANGE;
	}

	*size = offset;

err:
	log_trace("idx=%p, oid=%" PRIx64 ":%" PRIx64 ", size=%zu, rc=%d",
		  idx, oid->f_hi, oid->f_lo, *size, rc);
	kvs_itr_fini(kvstor, iter);
	return rc;
}

/** Frees a passed md pointer. */
void md_xattr_free(void *ptr)
{
	md_free(ptr);
}
