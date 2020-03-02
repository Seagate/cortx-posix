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

int nsal_init(void) 
{
	int rc = 0;
	struct collection_item *item = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);
	rc = kvs_init(kvstor, item);
	if (rc) {
                log_err("kvs_init failed");
                goto err;
        }
	rc = ns_init(item);
	if (rc) {
                log_err("ns_init failed");
                goto err;
        }
err:
        log_debug("rc=%d ", rc);
	return rc;
}

int nsal_fini()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();
        dassert(kvstor != NULL);
	rc = ns_fini();
	if (rc) {
                log_err("ns_fini failed");
                goto err;
        }
	rc = kvs_fini(kvstor);
	if (rc) {
                log_err("kvs_fini failed");
                goto err;
        }
err:
        log_debug("rc=%d ", rc);
        return rc;
}
