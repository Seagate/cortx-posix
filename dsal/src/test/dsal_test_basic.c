/*
 * Filename:		dsal_test_basic.c
 * Description:		Test group for very basic DSAL tests.
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

#include <stdio.h> /* *printf */
#include <memory.h> /* mem* functions */
#include <assert.h> /* asserts */
#include <errno.h> /* errno codes */
#include <stdlib.h> /* alloc, free */
#include "dstore.h" /* dstore operations to be tested */
#include "dstore_bufvec.h" /* data buffers and vectors */
#include "dsal_test_lib.h" /* DSAL-specific helpers for tests */

/*****************************************************************************/
/** Test environment for the test group.
 * The environment is prepared by setup() and cleaned up by teardown()
 * calls.
 */
struct env {
	/* Object ID to be used in the test cases.
	 * Rationale of keeping it global:
	 * 1. Each test case is responsibe for releasing this ID.
	 * 2. If test fails then the process goes down, so that
	 *    a new ID will be generated so that we won't get any collisions.
	 */
	dstore_oid_t oid;
	/* Dstore instance.
	 * Rationale of keeping it global:
	 * It is a singleton and it is being initialized
	 * only once. The initialization is not a part
	 * of any test case in this module.
	 */
	struct dstore *dstore;
};

#define ENV_FROM_STATE(__state) (*((struct env **) __state))

/*****************************************************************************/
/* Description: Test create and delete operation.
 * Strategy:
 *	Create a new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls.
 * Enviroment:
 *	Empty dstore.
 */
static void test_creat_del_basic(void **state)
{
	int rc;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
/* Description: Test creation of already existing object
 * Strategy:
 *	Create a new file
 *	Again create the same file
 *	In above step DSAL should return -EEXIST
 *	Delete the new file
 * Expected behavior:
 *	When we try to recreate the already existing file DSAL should give
 *	error with  -EEXIST return code
 * Enviroment:
 *	Empty dstore.
 */
static void test_creat_existing(void **state)
{
	int rc;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, -EEXIST);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
/* Description: Test deletion of non existing object
 * Strategy:
 *	Delete the non existing file
 * Expected behavior:
 *	 DSAL layer should return -ENOENT
 * Enviroment:
 *	Empty dstore.
 */
static void test_del_non_existing(void **state)
{
	int rc;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, -ENOENT);
}

/*****************************************************************************/
/* Description: Test Open and Close operations.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls.
 * Enviroment:
 *	Empty dstore.
 */
static void test_open_close_basic(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
/* Description: Open non-existing object.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Close the new file.
 *	Delete the new file.
 *	Open the deleted file
 * Expected behavior:
 *	No errors from the DSAL calls except ENOENT.
 * Enviroment:
 *	Empty dstore.
 */
static void test_open_non_existing(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);
	obj = NULL;

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, -ENOENT);
	ut_assert_null(obj);
}

/*****************************************************************************/
/* Description: Delete open object.
 * DSAL does not provide any guarantes regarding removal of open objects
 * and should not fail on close() call even if the object was removed.
 * The upper modules should detect and handle this scenario in accordance
 * with their state management policies.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Close the new file.
 *	Delete the new file.
 *	Open the deleted file
 * Expected behavior:
 *	No errors from the DSAL calls except ENOENT.
 * Enviroment:
 *	Empty dstore.
 */
static void test_delete_open(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);
	obj = NULL;
}

/*****************************************************************************/
/* Description: Test WRITE operation.
 * This function simply tests WRITE-related API functions and sanity
 * of the returned retcodes. It does not do any functional testing
 * against the contents of the dstore or the IO buffers.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Write a block of data into the new file.
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls.
 * Enviroment:
 *	Empty dstore.
 */
static void test_write_basic(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct dstore_io_op *wop = NULL;
	struct dstore_io_vec *data = NULL;
	struct dstore_io_buf *buf = NULL;
	struct env *env = ENV_FROM_STATE(state);
	void *raw_buf = NULL;
	const size_t buf_size = 4 << 10;
	const off_t offset = 0;

	raw_buf = calloc(1, buf_size);
	ut_assert_not_null(raw_buf);

	dtlib_fill_data_block(raw_buf, buf_size);

	rc = dstore_io_buf_init(raw_buf, buf_size, offset, &buf);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_buf2vec(&buf, &data);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_io_op_write(obj, data, NULL, NULL, &wop);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(wop);

	rc = dstore_io_op_wait(wop);
	ut_assert_int_equal(rc, 0);

	dstore_io_op_fini(wop);
	dstore_io_vec_fini(data);
	dstore_io_buf_fini(buf);
	free(raw_buf);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
static int test_group_setup(void **state)
{
	struct env *env;

	env = calloc(sizeof(struct env), 1);
	ut_assert_not_null(env);

	env->oid = *dtlib_def_obj();
	env->dstore = dtlib_dstore();

	*state = env;

	return SUCCESS;
}

static int test_group_teardown(void **state)
{
	struct env *env = ENV_FROM_STATE(state);

	free(env);
	*state = NULL;

	return SUCCESS;
}

/*****************************************************************************/
/* Entry point for test group execution. */
int main(int argc, char *argv[])
{
	int rc;

	struct test_case test_group[] = {
		ut_test_case(test_creat_del_basic, NULL, NULL),
		ut_test_case(test_creat_existing, NULL, NULL),
		ut_test_case(test_del_non_existing, NULL, NULL),
		ut_test_case(test_open_close_basic, NULL, NULL),
		ut_test_case(test_write_basic, NULL, NULL),
		ut_test_case(test_open_non_existing, NULL, NULL),
		ut_test_case(test_delete_open, NULL, NULL),
	};

	rc = dtlib_setup(argc, argv);
	if (rc) {
		printf("Failed to set up the test group environment");
		goto out;
	}
	rc = DSAL_UT_RUN(test_group, test_group_setup, test_group_teardown);
	dtlib_teardown();

out:
	return rc;
}
