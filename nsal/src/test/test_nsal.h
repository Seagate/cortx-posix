/*
 * Filename: test_nsal.h
 * Description: Declararions of functions used to test nsal
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

#ifndef TEST_NSAL_H
#define TEST_NSAL_H

#include <stdio.h>
#include <stdlib.h>
#include <ini_config.h>
#include "eos/eos_kvstore.h"
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <common/log.h>
#include "str.h"
#include <namespace.h>
#include <ut.h>

#define DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

struct collection_item *cfg_items;

/**
 * Does required nsal initialization to execute unit tests
 */
int ut_nsal_init();

/**
 * Finishes nsal initialization
 */
void ut_nsal_fini(void);

// Namespace Tests

/**
 * Test for namespace initialization.
 */
void test_init_ns();

/**
 *Test for namespce finish initialization.
 */
void test_fini_ns();

/**
 * Test for namespace create.
 */
void test_create_ns();

/**
 * Test for namespace delete.
 */
void test_delete_ns();

/**
 * Test for namspace scan.
 */
void test_scan_ns();

// iter test
// todo
#endif /*_TEST_NSAL_H */
