/*
 * Filename: efs_fs.h
 * Description:  Declararions of functions used to test efs_fs
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
#include "fs.h"
#include "namespace.h"
#include "ut.h"
#include "efs.h"

#define DEFAULT_CONFIG "/etc/efs/efs.conf"

struct ut_efs_params {
	struct efs_fs *efs_fs;
	efs_cred_t cred;
	efs_ino_t file_inode, current_inode, parent_inode;
	struct kvstore *kvstor;
	char *file_name;
	char *fs_name;
};

struct collection_item *cfg_items;

/**
 * Initialization for NS tests
 */
int ut_efs_fs_setup();

/*
 * Finishes initialization done for NS tests
 */ 
int ut_efs_fs_teardown();
#endif /* TEST_EFS_FS_H */
