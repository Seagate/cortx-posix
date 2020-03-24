/*
 * Filename:         dsal.h
 * Description:      Provides DSAL specific implementation.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

*/

#ifndef _DSAL_H
#define _DSAL_H

#include "dstore.h" /* header for dstore */

/* Initialize dsal.
 * 
 * @param cfg[] - collection item, int flags.
 * 
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int dsal_init(struct collection_item *cfg, int flags);

/* Finalize dsal.
 *  
 *  @param void.
 *  
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */

int dsal_fini(void);

#endif
