/*
 * Filename:         efs_internal.h
 * Description:      EOS file system internal APIs

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system internal APIs
*/

#ifndef _EFS_INTERNAL_H
#define _EFS_INTERNAL_H

#include "efs.h"
#include <dstore.h>

/** Set the extstore object identifier (kfid) with kvsns inode as the key */
int efs_set_ino_oid(struct efs_fs *efs_fs, efs_ino_t *ino, dstore_oid_t *oid);

/** Get the extstore object identifier for the passed efs inode */
int efs_ino_to_oid(struct efs_fs *efs_fs, const efs_ino_t *ino, dstore_oid_t *oid);

/** Delete the ino-kfid key-val pair from the kvs. Called during unlink/rm. */
int efs_del_oid(struct efs_fs *efs_fs, const efs_ino_t *ino);

#endif
