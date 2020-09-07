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

/* This API will write random data pattern of given size/offset for a file
 * read the given size/offset data from a file, validate the data integrity
 * and free up the allocated buffers
 */
static int test_write_read_validate(struct dstore_obj *obj,
                                    const size_t w_size,
                                    const off_t w_offset,
                                    const size_t r_size,
                                    const off_t r_offset,
                                    const size_t bsize)
{
	int rc;
	void *write_buf = NULL;
	void *read_buf = NULL;

	printf("test_write_read_validate w_size = %uk, w_offset = %uk "
	       "r_size = %uk, r_offset = %uk\n",
	       (unsigned int)w_size >> 10,
	       (unsigned int)w_offset >> 10,
	       (unsigned int)r_size >> 10,
	       (unsigned int)r_offset >> 10);

	write_buf = calloc(1, w_size);
	ut_assert_not_null(write_buf);

	read_buf = calloc(1, r_size);
	ut_assert_not_null(read_buf);

	dtlib_fill_data_block(write_buf, w_size);

	rc = dstore_io_op_pwrite(obj, w_offset, w_size, bsize, write_buf);
	ut_assert_int_equal(rc, 0);

	rc = dstore_io_op_pread(obj, r_offset, r_size, bsize, read_buf);
	ut_assert_int_equal(rc, 0);

	/* Data integrity check */
	rc = memcmp(write_buf, read_buf, w_size);
	ut_assert_int_equal(rc, 0);

	free(write_buf);
	free(read_buf);

	return rc;
}

/*****************************************************************************/
/* Description: Test WRITE/READ operation on different aligned buffer size.
 * This function write a [4k, 8k, 16k, 1M] block of data and read it back, does
 * the data integrity check of read data and expecting it to match with written
 * data
 * NOTE: Here test always start writing for new size from offset 0 and hence
 * earlier written data is over written with bigger buffer size, test is mostly
 * trying to make sure next time it does not write same set of data plus it is
 * making sure data integrity, So ultimately this test also prove that over
 * writing already written object and reading back is also working fine.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Write a specific size block of data into the new file.
 *	Read a specific size block of data from the file
 *	Compare the read block and written block of data
 *	Repeat write/read/validate step for rest of the block size
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls and data integrity
 *	check for read/write buffer should pass.
 * Enviroment:
 *	Empty dstore.
 */
static void test_write_read_aligned(void **state)
{
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);
	const off_t offset = 0;
	const size_t size_array_k[] = {4, 8, 16, 1024};
	const size_t bs = 4096;
	uint32_t num_element = sizeof(size_array_k)/sizeof(size_array_k[0]);
	uint32_t index;

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	for (index = 0; index < num_element; index++) {
		uint32_t buf_size = size_array_k[index] << 10;
		test_write_read_validate(obj, buf_size, offset,
					 buf_size, offset, bs);
	}

	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);
}

/*****************************************************************************/
/* Description: This test case will read a holes from a file and try to verify
 * the behaviour is as expected
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Write a file with block size[4k,8k,16k,32k,64k], iterate over each size
 *	one by one in a loop
 *	Read a file with two block read at a time, basically read ahead a block
 *	than what is written to the file
 *	Check return code for read
 *	1. If it return rc = 0, verify the data integrity against written data.
 *	2.rc = -ENOENT
 *	Motr is not able to handle the case where some part of object have
 *	not been written or created. For that it returns -ENOENT and zeroed out
 *	the data for all the read block even though some of them are available
 *	and we should get valid data for them atleast. For such case, this is
 *	the workaround where if we are reading more than one block size we will
 *	read all the block one by one so that for originally available block
 *	we will get proper data. So we will read block by block in this case
 *	Repeat Write/Read/Validate steps till written file size is 64k, this
 *	will ensure for each block size[4k,8k,..] we have some regressive loop.
 *	Repeat the same steps for other block size as mentioned earlier
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls and data integrity
 * Enviroment:
 *	Empty dstore.
 */
static void test_holes_in_file(void **state)
{
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);
	const size_t size_array_k[] = {4, 8, 16, 32, 64};
	uint32_t sizeidx, blkidx, num_sz_element;
	/* Let's limit the write to the file upto 64k for this test case */
	uint32_t file_size = 64 << 10;

	num_sz_element = sizeof(size_array_k)/sizeof(size_array_k[0]);

	for (sizeidx = 0; sizeidx < num_sz_element; sizeidx++) {
		size_t buf_sz = size_array_k[sizeidx] << 10;

		/* Calculate  the number of blocks to be written for
		 * for a given block size to write up to file size
		 */
		uint32_t num_blk = file_size / buf_sz;

		/* Create/Delete, Open/Close for every different block size
		 * to ensure we do not overwrite the existing written data
		 * and not reading back already written data which will make
		 * sure we are reading a holes from a file everytime
		 */
		test_create_file(env->dstore, &env->oid, 0);
		test_open_file(env->dstore, &env->oid, &obj, 0, true);

		for (blkidx = 0; blkidx < num_blk; blkidx++) {
			test_write_read_validate(obj,
						 buf_sz /*w_size*/,
						 (blkidx * buf_sz) /*w_offset*/,
						 (buf_sz * 2) /*r_size*/,
						 (blkidx * buf_sz)/*r_offset*/,
						 buf_sz /*bsize*/);
		}

		test_close_file(obj, 0);
		test_delete_file(env->dstore, &env->oid, 0);
	}
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
	struct env *env = ENV_FROM_STATE(state);
	void *raw_buf = NULL;
	const size_t buf_size = 4 << 10;
	const off_t offset = 0;

	raw_buf = calloc(1, buf_size);
	ut_assert_not_null(raw_buf);

	dtlib_fill_data_block(raw_buf, buf_size);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	rc = dstore_io_op_pwrite(obj, offset, buf_size, buf_size, raw_buf);
	ut_assert_int_equal(rc, 0);

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
 * No error from DSAL API and buffer shoud be filled with zero
 * Enviroment:
 *	Empty dstore.
 */
static void test_read_basic(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);
	void *raw_buf = NULL;
	const size_t buf_size = 4 << 10;
	const off_t offset = 0;

	raw_buf = calloc(1, buf_size);
	ut_assert_not_null(raw_buf);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);

	dtlib_fill_data_block(raw_buf, buf_size);
	rc = dstore_io_op_pread(obj, offset, buf_size, buf_size, raw_buf);
	ut_assert_int_equal(rc, 0);

	rc = dtlib_verify_data_block(raw_buf, buf_size, 0);
	ut_assert_int_equal(rc, 0);
	free(raw_buf);

	test_close_file(obj, 0);
	test_delete_file(env->dstore, &env->oid, 0);
}

/*****************************************************************************/
/* Description: Test READ operation on a deleted file.
 * This function verify the scenario where user read a deleted file
 * The expectation is user should be getting -ENOENT as return value.
 * DSAL should not fail on close for a deleted file
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Delete the new file.
 *	READ a block of data into the new file.
 *	Close the new file.
 * Expected behavior:
 * No error from DSAL API and buffer should be filled with zero
 * Enviroment:
 *	Empty dstore.
 */
static void test_read_deleted_file(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);
	void *raw_buf = NULL;
	const size_t buf_size = 4 << 10;
	const off_t offset = 0;

	raw_buf = calloc(1, buf_size);
	ut_assert_not_null(raw_buf);

	test_create_file(env->dstore, &env->oid, 0);
	test_open_file(env->dstore, &env->oid, &obj, 0, true);
	test_delete_file(env->dstore, &env->oid, 0);

	dtlib_fill_data_block(raw_buf, buf_size);
	rc = dstore_io_op_pread(obj, offset, buf_size, buf_size, raw_buf);
	ut_assert_int_equal(rc, 0);

	rc = dtlib_verify_data_block(raw_buf, buf_size, 0);
	ut_assert_int_equal(rc, 0);
	test_close_file(obj, 0);
	free(raw_buf);
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
		ut_test_case(test_read_deleted_file, NULL, NULL),
		ut_test_case(test_write_read_aligned, NULL, NULL),
		ut_test_case(test_holes_in_file, NULL, NULL),
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
