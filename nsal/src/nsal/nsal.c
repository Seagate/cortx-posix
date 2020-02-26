/*
 * Filename: nsal.c
 * Description: Implementation of Namesapce.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 *  Author: Satendra Singh <satendra.singh@seagate.com>
 */

#include <namespace.h> /*namespace*/
#include <common/log.h> /*logging*/

//TODO nsal_init is only placeholder, needs to be re-visit.
int nsal_init() 
{
	int rc = 0;
	struct collection_item *item;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor != NULL);
	//TODO kvstore_init needs to be finalize.
        kvstore_init(kvstor, items);
	ns_init();
	return rc;
}

//TODO nsal_fini is only placeholder, needs to be re-visit.
int nsal_fini()
{
	int rc = 0;
	ns_fini();
	kvstore_fini();
	return rc;
}
