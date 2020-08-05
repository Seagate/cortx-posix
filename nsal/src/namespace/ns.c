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

#include <errno.h> /* error no */
#include "debug.h" /* dassert */
#include "namespace.h" /* namespace */
#include "internal/nsal_common.h" /* nsal internal data structure */
#include "common.h" /* likely */
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "common/log.h" /* logging */

struct namespace {
	uint16_t ns_id; /*namespace object id, monotonically increments*/
	str256_t ns_name; /*namespace name*/
	kvs_idx_fid_t ns_fid; /*namespace index fid*/
};

/* This is a global NS index which stores information about all the namespace */
static struct kvs_idx g_ns_meta_index;

void ns_get_name(struct namespace *ns, str256_t **name)
{
	dassert(ns);
	*name = &ns->ns_name;
}

void ns_get_fid(struct namespace *ns, kvs_idx_fid_t *ns_fid)
{
	dassert(ns);
	*ns_fid = ns->ns_fid;
}

void ns_get_id(struct namespace *ns, uint16_t *ns_id)
{
	dassert(ns);
	*ns_id = ns->ns_id;
}

int ns_scan(void (*ns_scan_cb)(struct namespace *ns, size_t ns_size))
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

out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	kvs_itr_fini(kvstor, kvs_iter);
	log_debug("rc=%d", rc);

	return rc;
}

int ns_next_id(uint16_t *ns_id)
{
	int rc = 0;
	size_t buf_size = 0;
	struct key_prefix *key_prefix = NULL;
	uint16_t *val_ptr = NULL;
	uint16_t val = 0;
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

	log_debug("ctx=%p ns_id=%d rc=%d",
			g_ns_meta_index.index_priv, *ns_id, rc);

	return rc;
}

int ns_module_init(struct kvs_idx *meta_index)
{
	int rc = 0;

	dassert(meta_index != NULL);
	g_ns_meta_index = *meta_index;
	log_debug("Namespace module finalized. rc=%d", rc);
	return rc;
}

int ns_module_fini()
{
	log_debug("Namespace module finalized.");
	return 0;
}

int ns_create(const str256_t *name, struct namespace **ret_ns, size_t *ns_size)
{
	int rc = 0;
	uint16_t ns_id = 0;
	struct namespace *ns = NULL;
	struct kvs_idx ns_index;
	kvs_idx_fid_t index_fid = {0};
	struct ns_key *ns_key = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, str256_isalphanum, name);

	/* Get next ns_id */
	RC_WRAP_LABEL(rc, out, ns_next_id, &ns_id);
	/* prepare for namespace index */
	RC_WRAP_LABEL(rc, out, kvs_idx_gen_fid, kvstor, &index_fid);

	/* Create namespace index */
	RC_WRAP_LABEL(rc, out, kvs_index_create, kvstor, &index_fid, &ns_index);
	/* dump namespace in kvs */
	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&ns, sizeof(*ns));
	ns->ns_id = ns_id;
	ns->ns_name = *name;
	ns->ns_fid = index_fid;

	RC_WRAP_LABEL(rc, free_ns, kvs_alloc, kvstor, (void **)&ns_key, sizeof(*ns_key));

	NS_KEY_INIT(ns_key, ns_id, NS_KEY_TYPE_NS_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &g_ns_meta_index, ns_key,
			sizeof(struct ns_key), ns, sizeof(struct namespace));

	*ret_ns = ns;
	*ns_size = sizeof(struct namespace);
	goto out;

free_ns:
	if (ns) {
		kvs_free(kvstor, ns);
	}

free_key:
	if (ns_key) {
		kvs_free(kvstor, ns_key);
	}

out:
	log_debug("ns_id=%d rc=%d", ns_id, rc);

	return rc;
}

int ns_delete(struct namespace *ns)
{
	int rc = 0;
	uint16_t ns_id = 0;
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
	log_debug("ns_id=%d rc=%d\n", ns_id, rc);

	return rc;
}
