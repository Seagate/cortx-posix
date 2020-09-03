/*
 * Filename: ut_nsal_ns.h
 * Description: Declararions of functions used to test nsal
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
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

#define DEFAULT_CONFIG "/etc/cortx/cortxfs.conf"
#define CONF_FILE "/tmp/efs/build-nsal/test/ut/ut_nsal.conf"

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
