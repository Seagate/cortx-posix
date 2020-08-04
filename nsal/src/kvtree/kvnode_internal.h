/*
 * Filename:         kvnode_internal.h
 * Description:      kvnode - Data types and declarations for kvnode -
                     component of kvtree.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
*/

#ifndef _KVNODE_INTERNAL_H
#define _KVNODE_INTERNAL_H

/* On-disk structure for storing node basic attributes.
 * Uses cases: EFS file stats */
struct kvnode_basic_attr {
	uint16_t size;          /* size of node basic attributes buffer */
	char attr[0];           /* node basic attributes buffer */
};

#endif /* _KVNODE_INTERNAL_H */
