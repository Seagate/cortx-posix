/*
 * Filename:         efs.h
 * Description:      EOS file system interfaces

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system interfaces.
*/

#ifndef _EFS_H
#define _EFS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <utils.h>

/**
 * Start the efs library. This should be done by every thread using the library
 *
 * @note: this function will allocate required resources and set useful
 * variables to their initial value. As the programs ends efs_init() should be
 * invoked to perform all needed cleanups.
 * In this version of the API, it takes no parameter, but this may change in
 * later versions.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_init(const char *config);

/**
 * Finalizes the efs library.
 * This should be done by every thread using the library
 *
 * @note: this function frees what efs_init() allocates.
 *
 * @param: none (void param)
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int efs_fini(void);

/**
 * @todo : This is s/kvsns_fs_ctx_t/efs_fs_ctx_t
 * We need to comeup with proper efs_fs_ctx_t object.
 */
typedef void *efs_ctx_t;

/**
 * @todo : When efs_ctx_t is defined, EFS_NULL_FS_CTX,
 * will also be redefined.
 */
#define EFS_NULL_FS_CTX (NULL)

#define EFS_DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

#endif

