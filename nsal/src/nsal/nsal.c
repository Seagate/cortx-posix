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

#include <namespace.h> /*namespace*/
#include <common/log.h> /*logging*/
#include <common/helpers.h> /*RC_WRAP_LABEL*/
#include <nsal.h> /*nsal*/

static int nsal_initialized;

static int nsal_initialized;

int nsal_init(struct collection_item *cfg_items) 
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
	rc = ns_init(cfg_items);
	if (rc) {
                log_err("ns_init failed, rc=%d", rc);
		kvs_fini(kvstor);
                goto err;
        }
err:
	return rc;
}

int nsal_fini()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();
        dassert(kvstor != NULL);
	rc = ns_fini();
	if (rc) {
                log_err("ns_fini failed, rc=%d", rc);
                goto err;
        }
	rc = kvs_fini(kvstor);
	if (rc) {
                log_err("kvs_fini failed, rc=%d", rc);
                goto err;
        }
	RC_WRAP(kvstor->kvstore_ops->fini);
err:
        return rc;
}
