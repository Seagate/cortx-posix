/*
 * Filename:         dstore.h
 * Description:      data store module of DSAL

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains dstore framework infrastructure.
 There are currently 1 users of this infrastructure,
        1. eos

 This gives capability of data storing.
*/

#ifndef _DSTORE_INTERNAL_H
#define _DSTORE_INTERNAL_H

struct dstore {
	/* Type of dstore, currently eos supported */
	char *type;
	/* Config for the dstore specified type */
	struct collection_item *cfg;
	/* Operations supported by dstore */
	const struct dstore_ops *dstore_ops;
	/* Not used currently */
	int flags;
};

extern const struct dstore_ops eos_dstore_ops;

#endif
