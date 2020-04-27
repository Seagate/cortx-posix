/*
 * Filename: efs_io_ops.c
 * Description: Implementation tests for read and write
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Shraddha Shinde <shraddha.shinde@seagate.com>
*/

#include "efs_fs.h"
#define BLOCK_SIZE 4096
#define IO_ENV_FROM_STATE(__state) (*((struct ut_io_env **)__state))

struct ut_io_env {
	struct ut_efs_params ut_efs_obj;
	char *data;
	char *buf_in;
};

/**
 * Fill data_str block with given data
 */
static void ut_fill_data(char *data_str, int size, char data)
{
	int i;
	for(i =	0; i<size; i++) {
		data_str[i] = data;
	}
}

/**
 * Setup for io test
 * Description: Create file.
 * Strategy:
 *  1. Create file.
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. File creation should be successful.
 */
static int io_test_setup(void **state)
{
	int rc = 0;
	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	ut_efs_obj->file_name = "io_test_file";
	
	rc = ut_file_create(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for io test
 * Description: Delete file.
 * Strategy:
 *  1. Delete file.
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. File deletion should be successful.
 */
static int io_test_teardown(void **state)
{
	int rc = 0;

	rc = ut_file_delete(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Test for read from non-existing file
 * Description:read from non-existing file
 * Strategy:
 *  1. Read from non-existing file
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. Read should fail with error -ENOENT.
 */
static void test_r_nonexist_file(void **state)
{
	int rc = 0;
	char *buf_out;
	efs_file_open_t fd;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	//inode is 0LL to make sure file with this inode does not exist.
	fd.ino = 0LL;

	buf_out = calloc(sizeof(char), BLOCK_SIZE);
	ut_assert_not_null(buf_out);

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd, buf_out,
			BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, -ENOENT);

	free(buf_out);
}

/**
 * Test for write to non-existing file
 * Description: Write to non-existing file
 * Strategy:
 *  1. Write to non-existing file
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. Write should fail with error -ENOENT.
 */
static void test_w_nonexist_file(void **state)
{
	int rc = 0;
	efs_file_open_t fd;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	//inode is 0LL to make sure file with this inode does not exist.
	fd.ino = 0LL;

	rc = efs_write(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd,
			ut_io_obj->data, BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test for read write single block
 * Description: Write a block , read a block
 * Strategy:
 *  1. Write first block of file.
 *  2. Read first block of file
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. Read output matches Write input.
 */
static void test_rw_4k(void **state)
{
	int rc = 0;
	char *buf_out;
	efs_file_open_t fd;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	fd.ino = ut_efs_obj->file_inode;

	buf_out = calloc(sizeof(char), BLOCK_SIZE);
	ut_assert_not_null(buf_out);

	rc = efs_write(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd,
			ut_io_obj->buf_in, BLOCK_SIZE, 0);
	
	ut_assert_int_equal(rc, BLOCK_SIZE);
	rc = 0;

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd, buf_out,
			BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, BLOCK_SIZE);
	rc = 0;

	rc = memcmp(buf_out, ut_io_obj->buf_in, BLOCK_SIZE);

	ut_assert_int_equal(rc, 0);

	free(buf_out);
}

/**
 * Test for re-write single block
 * Description: Re-wrrite a block , read  block
 * Strategy:
 *  1. Write first block of file.
 *  2. Read first block of file.
 *  3. Rewrite first blockof file.
 *  4. Read first block of file.
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. Read output matches Write input.
 */
static void test_rewrite(void **state)
{
	int rc = 0;
	char *buf_out;
	efs_file_open_t fd;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	fd.ino = ut_efs_obj->file_inode;

	buf_out = calloc(sizeof(char), BLOCK_SIZE);
	ut_assert_not_null(buf_out);

	rc = efs_write(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd,
			ut_io_obj->data, BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, BLOCK_SIZE);
	rc = 0;

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd, buf_out,
			BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, BLOCK_SIZE);
	rc = 0;

	rc = memcmp(buf_out, ut_io_obj->data, BLOCK_SIZE);

	ut_assert_int_equal(rc, 0);

	rc = efs_write(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd,
			ut_io_obj->buf_in, BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, BLOCK_SIZE);
	rc = 0;

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd, buf_out,
			BLOCK_SIZE, 0);

	ut_assert_int_equal(rc, BLOCK_SIZE);
	rc = 0;

	rc = memcmp(buf_out, ut_io_obj->buf_in, BLOCK_SIZE);

	ut_assert_int_equal(rc, 0);

	free(buf_out);
}

/**
 * Test for read write for an unaligned portion of data.
 * Description: read write for an unaligned portion of data.
 * Strategy:
 *  1. Write first block + x bytes of file.
 *  2. Read first block + x bytes of file
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. Read output matches Write input.
 */
static void test_rw_4k_unalinged(void **state)
{
	int rc = 0;
	char *buf_out;
	size_t blk_size = BLOCK_SIZE + 11;
	efs_file_open_t fd;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	fd.ino = ut_efs_obj->file_inode;

	buf_out = calloc(sizeof(char), blk_size);
	ut_assert_not_null(buf_out);

	rc = efs_write(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd,
			ut_io_obj->buf_in, blk_size, 0);
	
	ut_assert_int_equal(rc, blk_size);
	rc = 0;

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd, buf_out,
			blk_size, 0);

	ut_assert_int_equal(rc, blk_size);
	rc = 0;

	rc = memcmp(buf_out, ut_io_obj->buf_in, blk_size);
	ut_assert_int_equal(rc, 0);

	free(buf_out);
}

/**
 * Description: Mixed IO ranges for READ and WRITE.
 * Strategy:
 *  1. Write (write_offset, write_size) block of data.
 *  2. Read (read_offset, read_size) block of data.
 * Expected behavior:
 *  1. No errors from the DSAL calls.
 *  2. Read output matches expected output.
 *
 * Note: This is helper function, not a test case
 */
static int rw_mixed_range(void **state, efs_file_open_t *fd, uint64_t w_offset,
				uint64_t w_size, uint64_t r_offset,
				uint64_t r_size)
{
	int rc = 0;
	char *buf_out;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	buf_out = calloc(sizeof(char), r_size);
	ut_assert_not_null(buf_out);

	rc = efs_write(ut_efs_obj->efs_fs, &ut_efs_obj->cred, fd,
			ut_io_obj->buf_in, w_size, w_offset);
	
	ut_assert_int_equal(rc, w_size);
	rc = 0;

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, fd, buf_out,
			r_size, r_offset);

	ut_assert_int_equal(rc, r_size);
	rc = 0;

	rc = memcmp(ut_io_obj->buf_in, buf_out, w_size);
	ut_assert_int_equal(rc, 0);

	rc = memcmp(ut_io_obj->data, buf_out + w_size, r_size - w_size);
	ut_assert_int_equal(rc, 0);

	free(buf_out);

	return rc;
}

/**
 * Test to read 16k holes from from medium size file
 * Description: Read 16k holes from file
 * Strategy:
 *  1. Write first block.
 *  2. Read first and second block (hole).
 *  3. Verfy output.
 *  4. Repeat for each of blocka in file.
 * Expected Behavior:
 *  1. No error fromEFS API.
 *  2. Holes read as zeros.
 */
static void test_r_16kholes_in_middle(void **state)
{
	int rc = 0, i;
	const uint64_t blk_sz = 16 << 10; /* 16KB block */
	const uint64_t nr_blocks = 16; /* 256KB total */
	efs_file_open_t fd;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);

	fd.ino = ut_efs_obj->file_inode;

	for (i = 0; i < nr_blocks; i++) {
		rc = rw_mixed_range(state, &fd, blk_sz * i, blk_sz, blk_sz * i,
					blk_sz * 2);
		ut_assert_int_equal(rc, 0);
	}
}

/**
 * Setup for io_ops test group.
 */
static int io_ops_setup(void **state)
{
	int rc = 0;
	struct ut_io_env *ut_io_obj = NULL;

	ut_io_obj = calloc(sizeof(struct ut_io_env), 1);

	ut_assert_not_null(ut_io_obj);

	ut_io_obj->data = calloc(sizeof(char), 16 <<10);

	ut_assert_not_null(ut_io_obj->data);

	ut_fill_data(ut_io_obj->data, 16 <<10, 0);

	ut_io_obj->buf_in = calloc(sizeof(char), 16 <<10);

	ut_fill_data(ut_io_obj->buf_in, 16 <<10, 'a');

	*state = ut_io_obj;

	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/*
 * Teardown for io_ops test group.
 */
static int io_ops_teardown(void **state)
{
	int rc = 0;
	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);

	rc = ut_efs_fs_teardown(state);
	ut_assert_int_equal(rc, 0);

	free(ut_io_obj->data);
	free(ut_io_obj->buf_in);
	free(*state);

	return rc;
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/efs/io_ops.log";

	printf("IO tests\n");

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_r_nonexist_file, NULL, NULL),
		ut_test_case(test_w_nonexist_file, NULL, NULL),
		ut_test_case(test_rw_4k, io_test_setup, io_test_teardown),
		ut_test_case(test_rewrite, io_test_setup, io_test_teardown),
		ut_test_case(test_rw_4k_unalinged, io_test_setup,
				io_test_teardown),
		ut_test_case(test_r_16kholes_in_middle, io_test_setup,
				io_test_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, io_ops_setup,
				io_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
