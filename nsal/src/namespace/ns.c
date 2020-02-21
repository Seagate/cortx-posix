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
#include <debug.h> /*dassert*/
#include <ini_config.h> /*config*/
#include <namespace.h> /*namespace*/
#include <common.h> /*likely*/
#include <common/helpers.h> /*RC_WRAP_LABEL*/
#include <common/log.h> /*logging*/

struct namespace {
        uint64_t nsobj_id; /*namespace object id, monotonically increments*/
        str256_t ns_name; /*namespace name*/
        kvs_fid_t nsobj_fid; /*namespace object fid*/
	struct kvs_idx nsobj_index; /*namespace object index*/
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
        uint32_t ns_id;
} __attribute((packed));

#define NS_KEY_INIT(_key, _ns_id, _ktype)               \
{                                                       \
	_key->ns_id = _ns_id,                           \
	_key->ns_prefix.k_type = _ktype,                \
	_key->ns_prefix.k_version = NS_VERSION_0;       \
}

#define NS_KEY_PREFIX_INIT(_key, _type)        	\
{                                               \
	_key->k_type = _type,     		\
	_key->k_version = NS_VERSION_0;         \
}

/* This is a global NS index which stores information about all the namespace */
static struct kvs_idx g_ns_index;
static char *ns_fid_str;

void get_ns_name(struct namespace *ns, str256_t **out)
{
	dassert(ns);
	*out = &ns->ns_name;
}

int ns_scan(void (*cb)(struct namespace *))
{
	int rc = 0;
	int i = 0;
	struct kvs_itr *kvs_iter = NULL;
	size_t klen, vlen;
	void *key = NULL;
	struct namespace *val = NULL;
	static const size_t psize = sizeof(struct key_prefix);

	struct kvstore *kvstor = kvstore_get();

	struct ns_key prefix;
	prefix.ns_prefix.k_type = NS_KEY_TYPE_NS_INFO;
	prefix.ns_prefix.k_version = NS_VERSION_0;

	rc = kvs_itr_find(kvstor, &g_ns_index, &prefix, psize, &kvs_iter);
	if (rc) {
			goto out;
	}

	for (i = 0; (rc == 0); ++i) {
		kvs_itr_get(kvstor, kvs_iter, &key, &klen, (void **)&val, &vlen);

		if (vlen != sizeof(struct namespace)) {
			log_err("Invalid namespace entry in the KVS\n");
			goto out;
		}

		cb(val);
		rc = kvs_itr_next(kvstor, kvs_iter);
	}
	if (rc) {
         rc = rc == -ENOENT ? 0 : rc;
	} else {
        rc = 0;
	}
out:
	kvs_itr_fini(kvstor, kvs_iter);
	log_debug("rc = %d", rc);
	return rc;
}

int ns_next_id(uint32_t *nsobj_id)
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

	rc = kvs_get(kvstor, &g_ns_index, key_prefix, sizeof(struct key_prefix),
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
	RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &g_ns_index, key_prefix,
			sizeof(struct key_prefix), (void *)&val, sizeof(val));
	*nsobj_id = val;

free_key:
	if (key_prefix) {
		kvs_free(kvstor, key_prefix);
	}

out:
	if (val_ptr) {
		kvs_free(kvstor, val_ptr);
	}

	log_debug("ctx=%p ns_id=%lu rc=%d",
			g_ns_index.index_priv, (unsigned long int)*nsobj_id, rc);

	return rc;
}

int ns_init(struct collection_item *cfg)
{
	int rc = 0;
	kvs_fid_t ns_fid;
	struct collection_item *item;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	if (cfg == NULL) {
		log_err("ns_init failed");
		return -EINVAL;
	}

	item = NULL;
	rc = get_config_item("kvstore", "ns_fid", cfg, &item);
	ns_fid_str = get_string_config_value(item, NULL);
	RC_WRAP_LABEL(rc, out, kvs_fid_from_str, ns_fid_str, &ns_fid);

	/* open g_ns_index */
	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_fid, &g_ns_index);

out:
	log_debug("rc=%d\n", rc);

	return rc;
}

int ns_fini()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, kvs_index_close, kvstor, &g_ns_index);

out:
	return rc;
}

int ns_create(str256_t *name, struct namespace **ret_ns)
{
	int rc = 0;
	uint32_t nsobj_id = 0;
	struct namespace *ns = NULL;
	struct kvs_idx nsobj_index;
	kvs_fid_t nsobj_fid = {0};
	struct ns_key *ns_key = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, str256_isalphanum, name);

	/* Get next nsobj_id */
	RC_WRAP_LABEL(rc, out, ns_next_id, &nsobj_id);
	/* prepare for namespace obj index */
	RC_WRAP_LABEL(rc, out, kvs_fid_from_str, ns_fid_str, &nsobj_fid);
	nsobj_fid.f_lo = nsobj_id;

	/* Create namespace obj index */
	RC_WRAP_LABEL(rc, out, kvs_index_create, kvstor, &nsobj_fid, &nsobj_index);

	/* dump namespace in kvs */
	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&ns, sizeof(*ns));
	ns->nsobj_id = nsobj_id;
	ns->ns_name = *name;
	ns->nsobj_fid = nsobj_fid;
	ns->nsobj_index = nsobj_index;

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&ns_key, sizeof(*ns_key));

	NS_KEY_INIT(ns_key, nsobj_id, NS_KEY_TYPE_NS_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &g_ns_index, ns_key,
			sizeof(struct ns_key), ns, sizeof(struct namespace));

	*ret_ns = ns;

free_key:
	if (ns_key) {
		kvs_free(kvstor, ns_key);
	}

out:
	log_debug("nsobj_id=%d rc=%d\n", nsobj_id, rc);

	return rc;
}

int ns_delete(struct namespace *ns)
{
	int rc = 0;
	uint32_t nsobj_id;
	struct ns_key *ns_key = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(ns != NULL);

	nsobj_id = ns->nsobj_id;
	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&ns_key, sizeof(*ns_key));

	NS_KEY_INIT(ns_key, nsobj_id, NS_KEY_TYPE_NS_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_del, kvstor, &g_ns_index, ns_key,
			sizeof(struct ns_key));
	/* Delete namespace object index */
	RC_WRAP_LABEL(rc, out, kvs_index_delete, kvstor, &(ns->nsobj_fid));

	kvs_free(kvstor, ns);

free_key:
	if (ns_key) {
		kvs_free(kvstor, ns_key);
	}

out:
	log_debug("nsobj_id=%d rc = %d\n", nsobj_id, rc);

	return rc;
}

