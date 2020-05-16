/*
 * Filename: test_ns.h
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

#include <errno.h>
#include <string.h>
#include "debug.h"
#include "common/log.h"
#include "str.h"
#include "namespace.h"
#include "ut.h"

#define DEFAULT_CONFIG "/etc/efs/efs.conf"

struct collection_item *cfg_items;

// Namespace Tests

/**
 * Test for namespace initialization.
 */
void test_ns_init();

/**
 *Test for namespce finish initialization.
 */
void test_ns_fini();

/**
 * Test for namespace create.
 */
void test_ns_create();

/**
 * Test for namespace delete.
 */
void test_ns_delete();

/**
 * Test for namspace scan.
 */
void test_ns_scan();

// iter test
// todo
#endif /*_TEST_NSAL_H */
