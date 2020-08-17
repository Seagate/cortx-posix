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

/* Do finalization of io_op, io_vec and io_buf
 * If any of the param is NULL then for that this
 * operation is noop.
 */
static void test_io_cleanup(struct dstore_io_op *iop,
                            struct dstore_io_vec *data,
                            struct dstore_io_buf *buf)
{
	if (iop) {
		dstore_io_op_fini(iop);
	}

	if (data) {
		dstore_io_vec_fini(data);
	}

	if (buf) {
		dstore_io_buf_fini(buf);
	}
}

/* Try to create a new file and compare the expected results */
static void test_create_file(struct dstore *dstore, dstore_oid_t *oid,
                             int expected_rc)
{
	int rc;
	rc = dstore_obj_create(dstore, NULL, oid);
	ut_assert_int_equal(rc, expected_rc);
}

/* Try to delete a file and compare the expected results */
static void test_delete_file(struct dstore *dstore, dstore_oid_t *oid,
                             int expected_rc)
{
	int rc;
	rc = dstore_obj_delete(dstore, NULL, oid);
	ut_assert_int_equal(rc, expected_rc);
}

/* Try to open a file and compare the expected results
 * If it is expected to successfully open a file then object should be
 * a valid object
 */
static void test_open_file(struct dstore *dstore, dstore_oid_t *oid,
                           struct dstore_obj **obj, int expected_rc,
                           bool obj_valid)
{
	int rc;
	rc = dstore_obj_open(dstore, oid, obj);
	ut_assert_int_equal(rc, expected_rc);

	if (obj_valid) {
		ut_assert_not_null(*obj);
	}else {
		/* We would not have expected a file to be opened
		 * successfully
		 */
		ut_assert_int_not_equal(expected_rc, 0);
		ut_assert_null(*obj);
	}
}

/* Try to close a file and compare the expected results */
static void test_close_file(struct dstore_obj *obj, int expected_rc)
{
	int rc;
	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, expected_rc);
}

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
	struct env *env = ENV_FROM_STATE(state);

	test_create_file(env->dstore, &env->oid, 0);
	test_delete_file(env->dstore, &env->oid, 0);
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
	struct env *env = ENV_FROM_STATE(state);

	test_create_file(env->dstore, &env->oid, 0);
	test_create_file(env->dstore, &env->oid, -EEXIST);
	test_delete_file(env->dstore, &env->oid, 0);
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
	struct env *env = ENV_FROM_STATE(state);

	test_delete_file(env->dstore, &env->oid, -ENOENT);
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
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);
	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);
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
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);
	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);

	obj = NULL;
	test_open_file(env->dstore, &env->oid, &obj, -ENOENT, false);
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
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);
	test_delete_file(env->dstore, &env->oid, 0);
	test_close_file(obj, 0);
}

/*****************************************************************************/
/* Description: Test WRITE/READ operation.
 * This function write a block of data and read it back, does the data integrity
 * check of read data and expecting it to match with written data
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Write a block of data into the new file.
 *	Read a block of data from the file
 *	Compare the read block and written block of data
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls and data integrity
 *	check for read/write buffer should pass.
 * Enviroment:
 *	Empty dstore.
 */
static void test_write_read(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct dstore_io_op *wop = NULL;
	struct dstore_io_op *rop = NULL;
	struct dstore_io_vec *data = NULL;
	struct dstore_io_buf *buf = NULL;
	struct env *env = ENV_FROM_STATE(state);
	void *write_buf = NULL;
	void *read_buf = NULL;
	const size_t buf_size = 4 << 10;
	const off_t offset = 0;

	write_buf = calloc(1, buf_size);
	ut_assert_not_null(write_buf);

	read_buf = calloc(1, buf_size);
	ut_assert_not_null(read_buf);

	dtlib_fill_data_block(write_buf, buf_size);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	/* Write operation */
	rc = dstore_io_buf_init(write_buf, buf_size, offset, &buf);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_buf2vec(&buf, &data);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_op_write(obj, data, NULL, NULL, &wop);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(wop);

	rc = dstore_io_op_wait(wop);
	ut_assert_int_equal(rc, 0);

	test_io_cleanup(wop, data, buf);

	/* Read operation */
	rc = dstore_io_buf_init(read_buf, buf_size, offset, &buf);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_buf2vec(&buf, &data);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_op_read(obj, data, NULL, NULL, &rop);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(rop);

	rc = dstore_io_op_wait(rop);
	ut_assert_int_equal(rc, 0);

	/* Data integrity check */
	rc = memcmp(write_buf, read_buf, buf_size);
	ut_assert_int_equal(rc, 0);

	test_io_cleanup(rop, data, buf);

	free(write_buf);
	free(read_buf);

	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);
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

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	rc = dstore_io_buf_init(raw_buf, buf_size, offset, &buf);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_buf2vec(&buf, &data);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_op_write(obj, data, NULL, NULL, &wop);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(wop);

	rc = dstore_io_op_wait(wop);
	ut_assert_int_equal(rc, 0);

	test_io_cleanup(wop, data, buf);
	free(raw_buf);

	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);
}

/*****************************************************************************/
/* Description: Test READ operation.
 * This function simply tests READ-related API functions and sanity
 * of the returned retcodes. It does not do any functional testing
 * against the contents of the dstore or the IO buffers.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	READ a block of data into the new file.
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	ENOENT for the block we are reading
 *	as it is not written yet.
 * Enviroment:
 *	Empty dstore.
 */
static void test_read_basic(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct dstore_io_op *rop = NULL;
	struct dstore_io_vec *data = NULL;
	struct dstore_io_buf *buf = NULL;
	struct env *env = ENV_FROM_STATE(state);
	void *raw_buf = NULL;
	const size_t buf_size = 4 << 10;
	const off_t offset = 0;

	raw_buf = calloc(1, buf_size);
	ut_assert_not_null(raw_buf);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	rc = dstore_io_buf_init(raw_buf, buf_size, offset, &buf);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_buf2vec(&buf, &data);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_op_read(obj, data, NULL, NULL, &rop);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(rop);

	rc = dstore_io_op_wait(rop);
	ut_assert_int_equal(rc, -ENOENT);

	test_io_cleanup(rop, data, buf);
	free(raw_buf);

	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);
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
		ut_test_case(test_read_basic, NULL, NULL),
		ut_test_case(test_open_non_existing, NULL, NULL),
		ut_test_case(test_delete_open, NULL, NULL),
		ut_test_case(test_write_read, NULL, NULL),
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
