/*
 * Filename: readdir_profiling.c
 * Description: Implementation tests for directory operartions
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Atita Shirwaikar <atita.shirwaikar@seagate.com>
*/

/* This file has the implementation for the following experiment.
 * - Create 1000 files
 * - List 1000 files
 * - Delete 100 files
 * - Calculate time taken for above operation
*/
#include "ut_efs_helper.h"
#define DIR_NAME_LEN_MAX 255
#define DIR_ENV_FROM_STATE(__state) (*((struct ut_dir_env **)__state))

struct ut_dir_env {
	struct ut_efs_params ut_efs_obj;
	char **name_list;
	int entry_cnt;
};

struct readdir_ctx {
	int index;
	char *readdir_array[1001];
};

/**
 * Call-back function for readdir
 */
static bool test_readdir_cb(void *ctx, const char *name, const efs_ino_t *ino)
{
	struct readdir_ctx *readdir_ctx = ctx;

	readdir_ctx->readdir_array[readdir_ctx->index++] = strdup(name);

	return true;
}

/**
 * Free readdir array
 */
static void readdir_ctx_fini(struct readdir_ctx *ctx)
{
	int i;
	for (i = 0; i < ctx->index; i++) {
		free(ctx->readdir_array[i]);
	}
}

/**
 * Verify readdir content
 */
static void verify_dentries(struct readdir_ctx *ctx, struct ut_dir_env *env,
				int entry_start)
{
	int i;
	ut_assert_int_equal(env->entry_cnt, ctx->index);

	for (i = 0; i < ctx->index; i++) {
		ut_assert_string_equal(env->name_list[entry_start + i],
					ctx->readdir_array[i]);
	}
}

/**
 * Setup for dir_ops test group
 */
static int dir_ops_setup(void **state)
{
	int rc = 0;

	struct ut_dir_env *ut_dir_obj = calloc(sizeof(struct ut_dir_env), 1);

	ut_assert_not_null(ut_dir_obj);

	ut_dir_obj->name_list = calloc(sizeof(char *), 1000);

	if(ut_dir_obj->name_list == NULL) {
		rc = -ENOMEM;
		ut_assert_true(0);
	}

	*state = ut_dir_obj;
	rc = ut_efs_fs_setup(state);

	ut_assert_int_equal(rc, 0);

	return rc;
}

/**
 * Teardown for dir_ops test group
 */
static int dir_ops_teardown(void **state)
{
	int rc = 0;

	rc = ut_efs_fs_teardown(state);
	ut_assert_int_equal(rc, 0);
	
	free(*state);

	return rc;
}


static int create_1000_files_teardown(void **state)
{
	int rc = 0, i=0;
	time_t start_time, end_time;

	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	time(&start_time);
	printf("\nDeleting 1000 Files:Start time %s\n",ctime(&start_time));

	/*Delete 1000 files*/
	for (i=1;i<1001;i++)
	{
		ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[i];
		rc = ut_file_delete(state);
		ut_assert_int_equal(rc, 0);
	}

	/*Delete Directory*/
	ut_dir_obj->ut_efs_obj.parent_inode = EFS_ROOT_INODE;
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_dir_delete(state);
	ut_assert_int_equal(rc,0);

	time(&end_time);
	printf("Deleting Files:End time %s\n",ctime(&end_time));

	for(i=0;i<1001;i++)
	{
		free(ut_dir_obj->name_list[i]);
	}
	
	return rc;
}

/**Test to create 1000 files**/
static int create_1000_files_setup(void **state)
{
 	int i=0,rc=0;
	time_t start_time, end_time;
	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

	ut_dir_obj->entry_cnt = 1000;

	/* Allocate space for 1000 entries */
	for(i=0;i<1001;i++)
	{
		ut_dir_obj->name_list[i]=malloc(sizeof(char*));
	}
	
	/* Create a directory under root*/
	strcpy(ut_dir_obj->name_list[0], "Test_Dir");
	printf("Test dir %s\n",ut_dir_obj->name_list[0]);
	ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[0];
	rc = ut_dir_create(state);
	ut_assert_int_equal(rc,0);

	time(&start_time);
	printf("\nStart time:Create 1000 files is %s\n", ctime(&start_time));

	ut_dir_obj->ut_efs_obj.parent_inode = ut_dir_obj->ut_efs_obj.file_inode;

	/* Create 1000 files */
	for (i=1;i<1001;i++)
	{
		snprintf(ut_dir_obj->name_list[i],sizeof(char*),"%d",i+1);
		ut_dir_obj->ut_efs_obj.file_name = ut_dir_obj->name_list[i];
		rc = ut_file_create(state);
		ut_assert_int_equal(rc,0);
	}
	time(&end_time);
	printf("End time:Create 1000 files is %s\n", ctime(&end_time));
	return rc;
}

static void read_1000_files(void **state)
{
	int rc = 0;
	time_t start_time, end_time;
	efs_ino_t dir_inode = 0LL;
	
	struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
	struct ut_efs_params *ut_efs_obj = &ut_dir_obj->ut_efs_obj;

	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	time(&start_time);
	printf("\nStart time:Lookup %s\n",ctime(&start_time));

	rc = efs_lookup(ut_efs_obj->efs_fs, &ut_efs_obj->cred,
			         &ut_efs_obj->current_inode, ut_dir_obj->name_list[0],
			         &dir_inode);
	ut_assert_int_equal(rc,0);
	time(&end_time);
	printf("End time:Lookup %s\n", ctime(&end_time));

	time(&start_time);
	printf("\nStart time:Read 1000 files %s\n", ctime(&start_time));
	rc = efs_readdir(ut_efs_obj->efs_fs, &ut_efs_obj->cred, &dir_inode,
				       test_readdir_cb, readdir_ctx);
	ut_assert_int_equal(rc, 0);
	time(&end_time);
	printf("End time:Read 1000 files %s\n", ctime(&end_time));
	verify_dentries(readdir_ctx, ut_dir_obj, 1);

	readdir_ctx_fini(readdir_ctx);
}
int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/eos/test/ut/ut_efs.log";

	printf("Directory tests\n");

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
		ut_test_case(read_1000_files, create_1000_files_setup, create_1000_files_teardown),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, dir_ops_setup,
				dir_ops_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
