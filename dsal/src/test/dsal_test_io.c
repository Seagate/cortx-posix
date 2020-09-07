/*
 * Filename:		dsal_test_io.c
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
/* Description: Test WRITE/READ operations for both aligned/unaligned pattern.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Execute different write/read test scenarios.
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls and data integrity
 *	check for read/write buffer should pass.
 * Enviroment:
 *	Empty dstore.
 */
static void test_aligned_unaligned_io(void **state)
{
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);
	off_t offset = 0;
	const size_t bs = 4096;
	size_t count = 0;

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	char *read_buf = NULL;
	char *write_buf = NULL;

	/* TEST CASE 1 */

	/* Write at offset 3000, count = 100 bytes.
	 * This will test non left aligned and insider block write
	 */
	count = 100;
	offset = 3000;
	write_buf = calloc(count, sizeof(char));
	memset(write_buf, 'A', count);
	int rc = dstore_io_op_pwrite(obj, offset, count, bs, write_buf);
	ut_assert_int_equal(rc, 0);

	/* read 100 bytes from offset 3000.
	 * This will test non left aligned and insider block read.
	 */
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);
	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);

	rc = memcmp(write_buf, read_buf, count);
	ut_assert_int_equal(rc, 0);

	/* Read bs 4096 from offset 0.
	 * This will test aligned read and if start, middle, end
	 * of a block are correctly written.
	 */
	free(read_buf);
	read_buf = calloc(bs, sizeof(char));
	offset = 0;
	rc = dstore_io_op_pread(obj, offset, bs, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 3000, 0);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+3000, 100, 'A');
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+3100, 996, 0);
	ut_assert_int_equal(rc, 0);

	/* TEST CASE 2 */
	free(write_buf);
	free(read_buf);
	offset = 3100;
	count = 2000;

	/* Write at offset 3100, count = 2000
	 * This will test non left aligned, non right aligned write
	 */

	write_buf = calloc(count, sizeof(char));
	memset(write_buf, 'B', count);
	rc = dstore_io_op_pwrite(obj, offset, count, bs, write_buf);
	ut_assert_int_equal(rc, 0);

	/* Read first two blocks and check the two block for data */
	count = 8192;
	offset = 0;
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);
	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 3000, 0);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+3000, 100, 'A');
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+3100, 2000, 'B');
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+5100, 3092, 0);
	ut_assert_int_equal(rc, 0);

	/* TEST CASE 3 */
	/* Write at offset 5100, count = 7188
	 * This will test non left aligned, right aligned write
	 */
	free(write_buf);
	free(read_buf);
	offset = 5100;
	count = 7188;
	write_buf = calloc(count, sizeof(char));
	memset(write_buf, 'C', count);
	rc = dstore_io_op_pwrite(obj, offset, count, bs, write_buf);
	ut_assert_int_equal(rc, 0);

	/* Read at offset 5100, count = 7188
	 * This will test non left aligned, right aligned read.
	 */
	count = 7188;
	offset = 5100;
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);
	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 7188, 'C');
	ut_assert_int_equal(rc, 0);

	/* Read second and third entire blocks and check data */
	free(read_buf);
	offset = 4096;
	count = 8192;
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);

	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 1004, 'B');
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+1004, 7188, 'C');
	ut_assert_int_equal(rc, 0);

	/* TEST CASE 4 */
	/* Write at offset 12888, count = 17000.
	 * This will test left aligned, non right aligned write
	 */
	free(write_buf);
	free(read_buf);
	offset = 12288;
	count = 17000;
	write_buf = calloc(count, sizeof(char));
	memset(write_buf, 'D', count);
	rc = dstore_io_op_pwrite(obj, offset, count, bs, write_buf);
	ut_assert_int_equal(rc, 0);

	/* Read from offset 12288, count = 17000
	 * This will test left aligned, non right aligned read
	 */
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);
	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 17000, 'D');
	ut_assert_int_equal(rc, 0);

	/* Read from offset 12288, count = 20480
	 * This will test left & right aligned read.
	 * and check data so far.
	 */
	free(read_buf);
	offset = 12288;
	count = 20480;
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);
	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 17000, 'D');
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+17000, 3480, 0);
	ut_assert_int_equal(rc, 0);

	/* TEST CASE 5 */
	/* Do aligned write offset = 40960, count = 4096
	*/
	free(write_buf);
	free(read_buf);
	offset = 40960;
	count = 4096;
	write_buf = calloc(count, sizeof(char));
	memset(write_buf, 'E', count);
	rc = dstore_io_op_pwrite(obj, offset, count, bs, write_buf);
	ut_assert_int_equal(rc, 0);

	/* Read at offset 32768, count = 12288
	 * We will test reading two holes in middle of
	 * data written so far. And check the last block.
	 */
	count = 12288;
	offset = 32768;
	read_buf = calloc(count, sizeof(char));
	memset(read_buf, 0, count);
	rc = dstore_io_op_pread(obj, offset, count, bs, read_buf);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf, 8192, 0);
	ut_assert_int_equal(rc, 0);
	rc = dtlib_verify_data_block(read_buf+8192, 4096, 'E');
	ut_assert_int_equal(rc, 0);

	free(read_buf);
	free(write_buf);
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
		ut_test_case(test_aligned_unaligned_io, NULL, NULL),
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
