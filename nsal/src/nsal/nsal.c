/*
 * Filename: nsal.c
 * Description: Implementation of NSAL.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 */

#include <errno.h> /* errno */
#include "namespace.h" /* namespace */
#include "common/log.h" /* logging */
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "nsal.h" /* nsal */
#include "tenant.h" /* nsal */

static int nsal_initialized;
struct kvs_idx g_ns_meta_index;

/* Helper function: this will open the namespace index.
 * @param cfg_item[in]: collection item.
 * @param meta_index[out]: index of namespace table.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
static int nsal_ns_open(struct collection_item *cfg_items,
			struct kvs_idx *ns_meta_index)
{
	int rc = 0;
	kvs_idx_fid_t ns_meta_fid;
	struct collection_item *item;
	struct kvstore *kvstor = kvstore_get();
	char *ns_meta_fid_str = NULL;

	dassert(kvstor != NULL);
	dassert(cfg_items != NULL);

	item = NULL;
	RC_WRAP_LABEL(rc, out, get_config_item, "kvstore", "ns_meta_fid",
		     cfg_items, &item);
	if (item == NULL) {
		rc = -EINVAL;
		goto out;
	}
	ns_meta_fid_str = get_string_config_value(item, &rc);
	if (rc != 0) {
		log_err("Failed to get string value of 'kvstore.ns_meta_fid'.");
		goto out;
	}
	/* Assume: get_string_config_value always returns
	 * a valid pointer if rc is zero.
	 */
	dassert(ns_meta_fid_str != NULL);

	RC_WRAP_LABEL(rc, out, kvs_fid_from_str, ns_meta_fid_str, &ns_meta_fid);

	/* open ns_meta_index */
	RC_WRAP_LABEL(rc, out, kvs_index_open, kvstor, &ns_meta_fid, ns_meta_index);

out:
	log_debug("rc=%d", rc);
	return rc;
}

/* Helper function: this will close the namespace.
 * @param meta_index[in]: index of namespace table.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
static int nsal_ns_close(struct kvs_idx *ns_meta_index)
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);
	RC_WRAP_LABEL(rc, out, kvs_index_close, kvstor, ns_meta_index);

out:
	log_debug("rc=%d",rc);
	return 0;
}

int nsal_module_init(struct collection_item *cfg_items)
{
	int rc = 0;
	dassert(nsal_initialized == 0);
	nsal_initialized++;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);

	rc = kvs_init(kvstor, cfg_items);
	if (rc) {
		log_err("kvs_init failed, rc=%d", rc);
		goto err;
	}

	rc = nsal_ns_open(cfg_items, &g_ns_meta_index);
	if (rc) {
		log_err("nsal_ns_open failed, rc=%d", rc);
		kvs_fini(kvstor);
		goto err;
	}

	rc = ns_module_init(&g_ns_meta_index);
	if (rc) {
		log_err("ns_module_init failed, rc=%d", rc);
		kvs_fini(kvstor);
		goto err;
	}

	rc = tenant_module_init(&g_ns_meta_index);
	if (rc) {
		log_err("ns_module_init failed, rc=%d", rc);
		kvs_fini(kvstor);
		goto err;
	}

err:
	log_debug("NSAL module initialized rc=%d", rc);
	return rc;
}

int nsal_module_fini()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();
	dassert(kvstor != NULL);

	rc = ns_module_fini();
	if (rc) {
		log_err("ns_module_fini failed, rc=%d", rc);
	}

	rc = tenant_module_fini();
	if (rc) {
		log_err("tenant_module_fini failed, rc=%d", rc);
	}

	rc = nsal_ns_close(&g_ns_meta_index);
	if (rc) {
		log_err("nsal_ns_close failed, rc=%d", rc);
	}

	rc = kvs_fini(kvstor);
	if (rc) {
		log_err("kvs_fini failed, rc=%d", rc);
	}

	log_debug("NSAL module finalized, rc=%d", rc);
	return rc;
}
