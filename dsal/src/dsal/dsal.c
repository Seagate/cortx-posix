/*
 * Filename:         dsal.c
 * Description:      Contains DSAL specific implementation.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

*/

#include "dsal.h"  /* header for dsal_init/dsal_fini */
#include <debug.h> /* dassert */

static int dsal_initialized;

int dsal_init(struct collection_item *cfg, int flags)
{
	dassert(dsal_initialized == 0);
	dsal_initialized++;
	return dstore_init(cfg, flags);
}

int dsal_fini(void)
{
	struct dstore *dstor = dstore_get();
        dassert(dstor != NULL);
	return dstore_fini(dstor);
}

