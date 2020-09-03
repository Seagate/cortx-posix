/*
 * Filename: ut_efs_io_bug_ops.c
 * Description: Implementation tests for read and write bugs
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

#include "ut_efs_helper.h"
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
 * Test for read from empty file
 * Description: Read first block from empty file
 * Strategy:
 *  2. Read first block of empty file
 * Expected Behavior:
 *  1. No errors from EFS API.
 *  2. Data read should be zeros.
 */
static void test_r_empty_file(void **state)
{
	int rc = 0;
	char *buf_out;
	efs_file_open_t fd;

	struct ut_io_env *ut_io_obj = IO_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_io_obj->ut_efs_obj;

	struct stat stat_in;
	stat_in.st_size = 0;

	fd.ino = ut_efs_obj->file_inode;

	buf_out = (char *)malloc(sizeof(char) * BLOCK_SIZE);
	ut_assert_not_null(buf_out);

	ut_fill_data(buf_out, BLOCK_SIZE, 'a');

	rc = efs_truncate(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			&ut_efs_obj->file_inode, &stat_in, STAT_SIZE_SET);
	ut_assert_int_equal(rc, 0);

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &fd, buf_out,
			BLOCK_SIZE, 0);

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

	rc = efs_read(ut_efs_obj->efs_fs, &ut_efs_obj->cred, fd, buf_out,
			r_size, r_offset);

	if (rc == r_size) {
		/* Data read from hole should match as expected */
		rc = memcmp(ut_io_obj->data, buf_out + w_size, r_size - w_size);
		ut_assert_int_equal(rc, 0);
	}
	else {
		/* No holes left in a file, we should be reading only written
		 * number of bytes
		 */
		ut_assert_int_equal(rc, w_size);
	}

	/* Data written should be there in read data bytes */
	rc = memcmp(ut_io_obj->buf_in, buf_out, w_size);
	ut_assert_int_equal(rc, 0);

	free(buf_out);

	return rc;
}

/**
 * Test to read 4k holes from from medium size file
 * Description: read 4k holes from from medium size file
 * Strategy:
 *  1. Write first block.
 *  2. Read first and second block (hole).
 *  3. Verify output.
 *  4. Repate for each of the blocks in the file.
 * Expected Behavior:
 *  1. No errors from the DSAL calls.
 *  2. Holes read as zeros.
 *  3. Read output matches expected output.
 *
 * Bug Description:
 *  1. Data read from 2nd 4k block dos not match
 *     with data written.
 *  2. When block size is 8k, test fails with 1st block
 *     not matching with data written.
 *  3. Same test when block size and hole size is 2k, 16k 
 *     passes  without any error.
 */
static void test_r_4kholes_in_middle(void **state)
{
	int rc = 0, i;
	const uint64_t blk_sz = 4 << 10; /* 4KB block */
	const uint64_t nr_blocks = 16; /* 64KB total */
	efs_file_open_t fd;

	struct ut_efs_params *ut_efs_obj = ENV_FROM_STATE(state);
	struct stat stat_in;

	fd.ino = ut_efs_obj->file_inode;

	stat_in.st_size = nr_blocks * blk_sz;

	rc = efs_truncate(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			  &ut_efs_obj->file_inode, &stat_in, STAT_SIZE_SET);

	ut_assert_int_equal(rc, 0);

	for (i = 0; i < nr_blocks; i++) {
		rc = rw_mixed_range(state, &fd, (blk_sz * i), blk_sz,
					(blk_sz * i), (blk_sz * 2));
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
	char *test_log = "/var/log/cortx/test/ut/ut_efs.log";

	printf("IO Bug tests\n");

	rc = ut_load_config(CONF_FILE);
	if (rc != 0) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_log = ut_get_config("efs", "log_path", test_log);

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		goto out;
	}

	struct test_case test_list[] = {
		ut_test_case(test_r_empty_file, io_test_setup, io_test_teardown),
		ut_test_case(test_r_4kholes_in_middle, io_test_setup,
				io_test_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, io_ops_setup,
				io_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
