/*
 * Filename: test_efs.h
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
#include "str.h"
#include "fs.h"
#include "namespace.h"
#include "ut.h"

#define DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

struct collection_item *cfg_items;

/**
 * Does required efs initialization to execute unit tests
 */
int ut_efs_init();

/**
 * Finishes nsal initialization
 */
void ut_efs_fini(void);


// efs_fs Tests

/**
 * Test for efs_fs initialization.
 */
void test_efs_fs_init();

/**
 *Test for efs_fs finish initialization.
 */
void test_efs_fs_fini();

/**
 * Test for efs_fs create.
 */
void test_efs_fs_create();

/**
 * Test for efs_fs delete.
 */
void test_efs_fs_delete();

/**
 * Test for efs_fs list.
 */
void test_efs_fs_scan();
#endif /* TEST_EFS_FS_H */
