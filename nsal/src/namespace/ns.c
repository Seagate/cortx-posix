/*
 * Filename: ns.c
 * Description: Metadata - Implementation of Namesapce API.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 *  Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 */

#include <errno.h> /*error no*/
#include "debug.h" /*dassert*/
#include "namespace.h" /*namespace*/
#include "common.h" /*likely*/
#include "common/helpers.h" /*RC_WRAP_LABEL*/
#include "common/log.h" /*logging*/

struct namespace {
        uint64_t ns_id; /*namespace object id, monotonically increments*/
        str256_t ns_name; /*namespace name*/
        kvs_fid_t ns_fid; /*namespace object fid*/
	struct kvs_idx ns_index; /*namespace object index*/
};

typedef enum ns_version {
        NS_VERSION_0 = 0,
        NS_VERSION_INVALID,
} ns_version_t;

/* namespace key types associated with particular version of ns. */
typedef enum ns_key_type {
        NS_KEY_TYPE_NS_INFO = 1, /* Key for storing namespace information. */
        NS_KEY_TYPE_NS_ID_NEXT,  /* Key for storing id of namespace. */
        NS_KEY_TYPE_INVALID,
} ns_key_type_t;

/* namespace key */
struct ns_key {
        struct key_prefix ns_prefix;
        uint32_t ns_meta_id;
} __attribute((packed));

#define NS_KEY_INIT(_key, _ns_meta_id, _ktype)               \
{                                                       \
	_key->ns_meta_id = _ns_meta_id,                           \
	_key->ns_prefix.k_type = _ktype,                \
	_key->ns_prefix.k_version = NS_VERSION_0;       \
}

#define NS_KEY_PREFIX_INIT(_key, _type)        	\
{                                               \
	_key->k_type = _type,     		\
	_key->k_version = NS_VERSION_0;         \
}

/* This is a global NS index which stores information about all the namespace */
static struct kvs_idx g_ns_meta_index;
static char *ns_meta_fid_str;

void ns_get_name(struct namespace *ns, str256_t **name)
{
	dassert(ns);
	*name = &ns->ns_name;
}

void ns_get_ns_index(struct namespace *ns, struct kvs_idx **ns_index)
{
	dassert(ns);
	*ns_index = &ns->ns_index;
}

int ns_scan(void (*ns_scan_cb)(struct namespace *, size_t ))
{
	int rc = 0;
	struct kvs_itr *kvs_iter = NULL;
	size_t klen, vlen;
	void *key = NULL;
	struct namespace *ns = NULL;
	struct ns_key prefix;
	static const size_t psize = sizeof(struct key_prefix);
	struct kvstore *kvstor = kvstore_get();

	NS_KEY_PREFIX_INIT((&prefix.ns_prefix), NS_KEY_TYPE_NS_INFO);

	rc = kvs_itr_find(kvstor, &g_ns_meta_index, &prefix, psize, &kvs_iter);
	if (rc) {
		goto out;
	}

	do {
		kvs_itr_get(kvstor, kvs_iter, &key, &klen, (void **)&ns, &vlen);

		if (vlen != sizeof(struct namespace)) {
			log_err("Invalid namespace entry in the KVS\n");
			continue;
		}

		ns_scan_cb(ns, sizeof(struct namespace));
	} while ((rc = kvs_itr_next(kvstor, kvs_iter)) == 0);

	if (rc == -ENOENT) {
		rc = 0;
	}

out:
	kvs_itr_fini(kvstor, kvs_iter);
	log_debug("rc = %d", rc);

	return rc;
}

int ns_next_id(uint32_t *ns_id)
{
	int rc = 0;
	size_t buf_size = 0;
	struct key_prefix *key_prefix = NULL;
	uint32_t *val_ptr = NULL;
	uint32_t val = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&key_prefix,
			sizeof(struct key_prefix));

	NS_KEY_PREFIX_INIT(key_prefix, NS_KEY_TYPE_NS_ID_NEXT);

	rc = kvs_get(kvstor, &g_ns_meta_index, key_prefix, sizeof(struct key_prefix),
			(void **)&val_ptr, &buf_size);

	if (likely(rc == 0)) {
		dassert(val_ptr);
		val = *val_ptr;
	} else if (rc == -ENOENT) {
		dassert(val_ptr == NULL);
		val = NS_ID_INIT; // where NS_ID_INIT is 2.
	} else {
		// got an unexpected error
		goto free_key;
	}

	val++;
	RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &g_ns_meta_index, key_prefix,
			sizeof(struct key_prefix), (void *)&val, sizeof(val));
	*ns_id = val;

free_key:
	if (key_prefix) {
		kvs_free(kvstor, key_prefix);
	}

out:
	if (val_ptr) {
		kvs_free(kvstor, val_ptr);
	}

	log_debug("ctx=%p ns_meta_id=%lu rc=%d",
			g_ns_meta_index.index_priv, (unsigned long int)*ns_id, rc);

	return rc;
}

int ns_init(struct collection_item *cfg)
{
	int rc = 0;
	kvs_fid_t ns_meta_fid;
	struct collection_item *item;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	if (cfg == NULL) {
		log_err("ns_init failed");
		return -EINVAL;
	}

	item = NULL;
	rc = get_config_item("kvstore", "ns_meta_fid", cfg, &item);
	ns_meta_fid_str = get_string_config_value(item, NULL);
	RC_WRAP_LABEL(rc, out, kvs_fid_from_str, ns_meta_fid_str, &ns_meta_fid);

	/* open g_ns_meta_index */
	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_meta_fid, &g_ns_meta_index);

out:
	log_debug("rc=%d\n", rc);

	return rc;
}

int ns_fini()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, kvs_index_close, kvstor, &g_ns_meta_index);

out:
	return rc;
}

int ns_create(const str256_t *name, struct namespace **ret_ns, size_t *ns_size)
{
	int rc = 0;
	uint32_t ns_id = 0;
	struct namespace *ns = NULL;
	struct kvs_idx ns_index;
	kvs_fid_t ns_fid = {0};
	struct ns_key *ns_key = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, str256_isalphanum, name);

	/* Get next ns_id */
	RC_WRAP_LABEL(rc, out, ns_next_id, &ns_id);
	/* prepare for namespace obj index */
	RC_WRAP_LABEL(rc, out, kvs_fid_from_str, ns_meta_fid_str, &ns_fid);
	ns_fid.f_lo = ns_id;

	/* Create namespace obj index */
	RC_WRAP_LABEL(rc, out, kvs_index_create, kvstor, &ns_fid, &ns_index);

	/* dump namespace in kvs */
	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&ns, sizeof(*ns));
	ns->ns_id = ns_id;
	ns->ns_name = *name;
	ns->ns_fid = ns_fid;
	ns->ns_index = ns_index;

	RC_WRAP_LABEL(rc, free_ns, kvs_alloc, kvstor, (void **)&ns_key, sizeof(*ns_key));

	NS_KEY_INIT(ns_key, ns_id, NS_KEY_TYPE_NS_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &g_ns_meta_index, ns_key,
			sizeof(struct ns_key), ns, sizeof(struct namespace));

	*ret_ns = ns;
	*ns_size = sizeof(struct namespace);

free_ns:
	if (ns) {
		kvs_free(kvstor, ns);
	}

free_key:
	if (ns_key) {
		kvs_free(kvstor, ns_key);
	}

out:
	log_debug("ns_id=%d rc=%d\n", ns_id, rc);

	return rc;
}

int ns_delete(struct namespace *ns)
{
	int rc = 0;
	uint32_t ns_id;
	struct ns_key *ns_key = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(ns != NULL);

	ns_id = ns->ns_id;

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&ns_key, sizeof(*ns_key));

	NS_KEY_INIT(ns_key, ns_id, NS_KEY_TYPE_NS_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_del, kvstor, &g_ns_meta_index, ns_key,
			sizeof(struct ns_key));

	/* Delete namespace object index */
	RC_WRAP_LABEL(rc, out, kvs_index_delete, kvstor, &ns->ns_fid);

	kvs_free(kvstor, ns);

free_key:
	if (ns_key) {
		kvs_free(kvstor, ns_key);
	}

out:
	log_debug("ns_id=%d rc = %d\n", ns_id, rc);

	return rc;
}
