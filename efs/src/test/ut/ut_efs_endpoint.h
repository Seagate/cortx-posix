/*
 * Filename: efs_export.h
 * Description:  Declararions of functions used to test efs_export
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 */

#ifndef TEST_EFS_FS_H
#define TEST_EFS_FS_H

#include <string.h>
#include <errno.h>
#include <dstore.h>
#include "str.h"
#include "internal/fs.h"
#include "namespace.h"
#include "ut.h"
#include "efs.h"

#define DEFAULT_CONFIG "/etc/cortx/cortxfs.conf"

struct ut_efs_params {
	efs_fs_ctx_t fs_ctx;
	efs_cred_t cred;
	efs_ino_t file_inode, current_inode, parent_inode;
	struct kvstore *kvstor;
	char *file_name;
	char *fs_name;
};

struct collection_item *cfg_items;

#endif /* TEST_EFS_FS_H */
