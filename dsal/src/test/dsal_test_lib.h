/*
 * Filename:		dsal_test_lib.h
 * Description:		Defines a set of functions used across the DSAL
 *			unit tests modules (dsal_test_*.c).
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 *
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

/* Get initialized dstore instance. */
struct dstore *dtlib_dstore(void);

/** Get default object ID. */
const dstore_oid_t *dtlib_def_obj(void);

/** Global setup action. */
void dtlib_setup(int argc, char *argv[]);

/* Global teardown action. */
void dtlib_teardown(void);

/* Fill buffer with some non-zero data. */
static inline void dtlib_fill_data_block(uint8_t *data, size_t size)
{
	uint8_t value = 0;
	size_t i;

	for (i = 0; i < size; i++) {
		data[i] = value;
		value++;
	}
}

#define DSAL_UT_RUN(_test_group, _setup, _teardown) ut_run(_test_group,\
	sizeof(_test_group)/sizeof(_test_group[0]), _setup, _teardown)

/******************************************************************************/
#endif /* DSAL_TEST_LIB_H_ */
