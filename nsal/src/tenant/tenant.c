/*
 * Filename: tenant.c
 * Description: Metadata - Implementation of Tenant API.
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
#include <string.h> /* memcpy */
#include "debug.h" /* dassert */
#include "tenant.h" /* tenant */
#include "internal/nsal_common.h" /* nsal internals */
#include "common.h" /* likely */
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "common/log.h" /* logging */

struct tenant {
	uint16_t tenant_id; /*tenant object id, same as namespace id */
	str256_t tenant_name; /*tenant name*/
	uint16_t size; /* size for priv */
	char priv[0]; /* tenant info */
};

/* This is a global TENANT index which stores information about all the tenant */
static struct kvs_idx g_tenant_meta_index;

void tenant_get_name(struct tenant *tenant, str256_t **name)
{
	dassert(tenant);
	*name = &tenant->tenant_name;
}

void tenant_get_info(struct tenant *tenant, void **info)
{
	dassert(tenant);
	*info = tenant->priv;
}

int tenant_copy(const struct tenant *src, struct tenant **dest)
{
	int rc = 0;
	int tot_length = sizeof(struct tenant) + src->size;

	dassert(src);

	*dest = malloc(tot_length);
	if (*dest == NULL) {
		log_err("Cannot allocate tenant object");
		rc = -ENOMEM;
		goto out;
	}
	memcpy(*dest, src, tot_length);
out:
	return rc;
}

void tenant_free(struct tenant *obj)
{
	if(obj) {
		free(obj);
	}
}

int tenant_scan(int (*tenant_scan_cb)(void *cb_ctx, struct tenant *tenant),
		void *cb_ctx)
{
	int rc = 0;
	struct kvs_itr *kvs_iter = NULL;
	size_t klen, vlen;
	void *key = NULL;
	struct tenant *tenant = NULL;
	struct tenant_key prefix;
	static const size_t psize = sizeof(struct key_prefix);
	struct kvstore *kvstor = kvstore_get();

	TENANT_KEY_PREFIX_INIT((&prefix.tenant_prefix), NS_KEY_TYPE_TENANT_INFO);

	RC_WRAP_LABEL(rc, out, kvs_itr_find, kvstor, &g_tenant_meta_index,
		      &prefix, psize, &kvs_iter);

	do {
		kvs_itr_get(kvstor, kvs_iter, &key, &klen, (void **)&tenant, &vlen);
		if (vlen != sizeof(struct tenant) + tenant->size) {
			log_err("Invalid tenant entry in the KVS\n");
			cb_ctx = (void *)-1;
			rc = -EINVAL;
		}

		RC_WRAP_LABEL(rc, out, tenant_scan_cb, cb_ctx, tenant);
	} while ((rc = kvs_itr_next(kvstor, kvs_iter)) == 0);

out:
	if (rc == -ENOENT) {
		rc = 0;
	}

	kvs_itr_fini(kvstor, kvs_iter);
	log_debug("rc=%d", rc);

	return rc;
}

int tenant_module_init(struct kvs_idx *meta_index)
{
	int rc = 0;

	dassert(meta_index != NULL);
	g_tenant_meta_index = *meta_index;

	log_debug("Tenants module initialized, rc=%d", rc);
	return rc;
}

int tenant_module_fini(void)
{
	log_debug("Tenants module finalized.");
	return 0;
}

int tenant_create(const str256_t *name, struct tenant **ret_tenant, uint16_t ns_id,
		  const char *options)
{
	int rc = 0;
	struct tenant *tenant = NULL;
	struct tenant_key *tenant_key = NULL;
	struct kvstore *kvstor = kvstore_get();
	int len = 0;
	int tot_length = 0;

	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, str256_isalphanum, name);
	/* dump tenant in kvs */
	len = strlen(options);
	tot_length = sizeof(struct tenant) + strlen(options);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&tenant, tot_length);
	tenant->tenant_id = ns_id;
	tenant->tenant_name = *name;

	/* fill option here */
	memcpy(tenant->priv, options, len);
	tenant->size = len;
	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&tenant_key,
		      sizeof(*tenant_key));

	TENANT_KEY_INIT(tenant_key, ns_id, NS_KEY_TYPE_TENANT_INFO);

	RC_WRAP_LABEL(rc, out, kvs_set, kvstor, &g_tenant_meta_index, tenant_key,
			sizeof(struct tenant_key), tenant, tot_length);

	*ret_tenant = tenant;
	tenant = NULL;

out:
	if (tenant) {
		kvs_free(kvstor, tenant);
	}

	if (tenant_key) {
		kvs_free(kvstor, tenant_key);
	}

	log_debug(STR256_F " tenant(%p) tenant_id=%d rc=%d", STR256_P(name),
		  tenant, ns_id, rc);
	return rc;
}

int tenant_delete(struct tenant *tenant)
{
	int rc = 0;
	uint16_t tenant_id = 0;
	struct tenant_key *tenant_key = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(tenant != NULL);

	tenant_id = tenant->tenant_id;

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&tenant_key, sizeof(*tenant_key));

	TENANT_KEY_INIT(tenant_key, tenant_id, NS_KEY_TYPE_TENANT_INFO);

	RC_WRAP_LABEL(rc, free_key, kvs_del, kvstor, &g_tenant_meta_index, tenant_key,
			sizeof(struct tenant_key));

free_key:
	if (tenant_key) {
		kvs_free(kvstor, tenant_key);
	}

out:
	log_debug(STR256_F " tenant(%p) tenant_id=%d rc=%d",
		  STR256_P(&tenant->tenant_name), tenant, tenant_id, rc);

	return rc;
}
