/*
 * Filename:		dsal_test_lib.h
 * Description:		Defines a set of functions used across the DSAL
 *			unit tests modules (dsal_test_*.c).
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

/*
 * Defines a small set of functions to be used across all the testcases/groups
 * for DSAL layer (dsal test library == dtlib).
 */

#ifndef DSAL_TEST_LIB_H_
#define DSAL_TEST_LIB_H_
/******************************************************************************/
#include <dstore.h> /* objid and dstore */
#include <stddef.h> /* size_t */
#include <stdint.h> /* uint*_t */

#include "ut.h"

/* Number of objects for stress test */
#define NUM_OF_OBJECTS 10

/* return values */
#define SUCCESS 0
#define FAILURE 1

/* Get initialized dstore instance. */
struct dstore *dtlib_dstore(void);

/** Get default object ID. */
const dstore_oid_t *dtlib_def_obj(void);
/** Get multiple object IDs. */
void dtlib_get_objids(dstore_oid_t[]);
/** Get client & server names - related to m0_filesystem_stats. */
void dtlib_get_clnt_svr(char **svr, char **clnt);

/** Global setup action. */
int dtlib_setup(int argc, char *argv[]);
int dtlib_setup_for_multi(int argc, char *argv[]);

/* Global teardown action. */
void dtlib_teardown(void);

/* Fill buffer with some non-zero data. */
static inline void dtlib_fill_data_block(uint8_t *data, size_t size)
{
	uint8_t value;
	size_t i;

	/* Let's initialize the given buffer with random pattern */
	srand(time(NULL));
	value = rand() % UINT8_MAX;

	for (i = 0; i < size; i++) {
		data[i] = value;
		value++;
	}
}

#define DSAL_UT_RUN(_test_group, _setup, _teardown) ut_run(_test_group,\
	sizeof(_test_group)/sizeof(_test_group[0]), _setup, _teardown)

/******************************************************************************/
#endif /* DSAL_TEST_LIB_H_ */
