/*
 * Filename: md_common.c
 * Description: Metadata - Implementation of common apis used by NSAL.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 *
 * Author: Subhash Arya <subhash.arya@seagate.com>
 */

#include <md_common.h>

void md_free(void *ptr)
{
	struct kvstore *kvstor = kvstore_get();

	MD_DASSERT(kvstor != NULL);
	kvstor->kvstore_ops->free(ptr);
}
