/*
 * Filename: getattr_profoling.c
 * Description: Implementation tests for file attributes
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
 * - set and get attributes for NUM_FILES
 * - Calculate time taken to get attributes for NUM_FILES
*/

#include "ut_efs_helper.h"
#include <sys/time.h>
#define NUM_FILES 100
#define DIR_ENV_FROM_STATE(__state) (*((struct ut_dir_env **)__state))

struct ut_dir_env{
struct ut_efs_params ut_efs_objs;
char **name_list;
efs_ino_t *file_inode;
};

/**
 * Test to set creation time
 * Description: Set creation time for file.
 * Strategy:
 *  1. Set ctime
 *  2. Verify setting ctime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. ctime recieved from getattr should match set ctime.
 */
static void set_ctime(void **state)
{
   struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
   struct ut_efs_params *ut_efs_objs = &ut_dir_obj->ut_efs_objs;
   time_t  set_time, start_time, end_time;

   int rc = 0, i = 0;
   int flag = STAT_CTIME_SET;

   time_t new_ctime;
   time(&new_ctime);

   struct tm *now_tm;
   now_tm = localtime(&new_ctime);

   // Increment current time by random value(1 hour in this case)
   now_tm->tm_hour += 1;

   new_ctime = mktime(now_tm);

   struct stat stat_in, stat_out;
   struct timeval st,et;
   time(&start_time);
   printf("Test 1 Start time %s\n",ctime(&start_time));	
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_in, 0, sizeof(stat_in));

      stat_in.st_ctim.tv_sec = new_ctime;
      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                     &ut_efs_objs->file_inode, &stat_in, flag);
      ut_assert_int_equal(rc, 0);
   }

   time(&set_time);
   printf("Time after setting attributes %s\n",ctime(&set_time));

   gettimeofday(&st,NULL);
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_out, 0, sizeof(stat_out));
      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                    &ut_efs_objs->file_inode, &stat_out);
      ut_assert_int_equal(rc, 0);

      ut_assert_int_equal(0, difftime(new_ctime, stat_out.st_ctime));
   }
   gettimeofday(&et,NULL);
   int elapsed = ((et.tv_sec - st.tv_sec)*1000000)+(et.tv_usec-st.tv_usec);
   printf("Get Attribute took %d seconds\n", elapsed);

   time(&end_time);
   printf("Test-1 End time %s\n",ctime(&end_time));	
}


/**
 * Test to set modification time
 * Description: Set modification time for file.
 * Strategy:
 *  1. Set mtime
 *  2. Verify setting mtime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. mtime recieved from getattr should match set mtime.
 */
static void set_mtime(void **state)
{
   struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
   struct ut_efs_params *ut_efs_objs = &ut_dir_obj->ut_efs_objs;

   time_t start_time, end_time, set_time;
   struct timeval st,et;
   int rc = 0,i;
   int flag = STAT_MTIME_SET;

   time_t new_mtime, cur_time;
   time(&new_mtime);

   struct tm *now_tm;
   now_tm = localtime(&new_mtime);

   // Increment mtime by random value(1 hour in this case)
   now_tm->tm_hour += 1;

   new_mtime = mktime(now_tm);

   struct stat stat_in,stat_out;

   time(&start_time);
   printf("Test-2 Start time %s\n",ctime(&start_time));	
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_in, 0, sizeof(stat_in));

      stat_in.st_mtim.tv_sec = new_mtime;

      time(&cur_time);
      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                   &ut_efs_objs->file_inode, &stat_in, flag);

      ut_assert_int_equal(rc, 0);
   }
   time(&set_time);
   printf("Test2 set time %s",ctime(&set_time));

   gettimeofday(&st,NULL);
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_out, 0, sizeof(stat_out));
      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
				            &ut_efs_objs->file_inode, &stat_out);

      ut_assert_int_equal(rc, 0);
   }
   gettimeofday(&et,NULL);
   int elapsed = ((et.tv_sec - st.tv_sec)*1000000)+(et.tv_usec-st.tv_usec);
   printf("Get Attribute took %d seconds\n", elapsed);

   time(&end_time);
   printf("Test 2 End time %s\n",ctime(&end_time));	
}

/**
 * Test to set access time
 * Description: Set access time for file.
 * Strategy:
 *  1. Set atime
 *  2. Verify setting atime by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. atime recieved from getattr should match set atime.
 */
static void set_atime(void **state)
{
   struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
   struct ut_efs_params *ut_efs_objs = &ut_dir_obj->ut_efs_objs;
   int rc = 0,i;
   int flag = STAT_ATIME_SET;
   time_t start_time, end_time, set_time;
   struct timeval st,et;

   time_t new_atime, cur_time;
   time(&new_atime);

   struct tm *now_tm;
   now_tm = localtime(&new_atime);

   // Increment atime by random value(1 hour in this case)
   now_tm->tm_hour += 1;

   new_atime = mktime(now_tm);

   struct stat stat_in, stat_out;
   time(&start_time);
   printf("Test 3 Start time %s\n",ctime(&start_time));	
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_in, 0, sizeof(stat_in));
      stat_in.st_atim.tv_sec = new_atime;

      time(&cur_time);

      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                      &ut_efs_objs->file_inode, &stat_in, flag);
      ut_assert_int_equal(rc, 0);
   }
   time(&set_time);
   printf("Test3 set time %s",ctime(&set_time));

   gettimeofday(&st,NULL);
   for (i=0;i<NUM_FILES;i++)
   {
      memset(&stat_out, 0, sizeof(stat_out));
      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                      &ut_efs_objs->file_inode, &stat_out);
      ut_assert_int_equal(rc, 0);
   }
   gettimeofday(&et,NULL);
   int elapsed = ((et.tv_sec - st.tv_sec)*1000000)+(et.tv_usec-st.tv_usec);
   printf("Get Attribute took %d seconds\n", elapsed);

   time(&end_time);
   printf("Test 3 End time %s\n",ctime(&end_time));	
}

/**
 * Test to set gid
 * Description: Set gid for file.
 * Strategy:
 *  1. Set gid
 *  2. Verify setting gid by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. gid recieved from getattr should match set gid.
 */
static void set_gid(void **state)
{
   struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
   struct ut_efs_params *ut_efs_objs = &ut_dir_obj->ut_efs_objs;
   int rc = 0,i;
   int flag = STAT_GID_SET;
   time_t start_time, end_time;
   struct timeval st,et;

   gid_t new_gid =100;

   time_t cur_time;

   struct stat stat_in, stat_out;
   time(&start_time);
   printf("Test 4 Start time %s\n",ctime(&start_time));

   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_in, 0, sizeof(stat_in));
      stat_in.st_gid = new_gid;
      
      time(&cur_time);

      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                      &ut_efs_objs->file_inode, &stat_in, flag);

      ut_assert_int_equal(rc, 0);
   }

   gettimeofday(&st,NULL);
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_out, 0, sizeof(stat_out));
      rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                     &ut_efs_objs->file_inode, &stat_out);

      ut_assert_int_equal(rc, 0);

      ut_assert_int_equal(stat_out.st_gid, new_gid);
   }
   gettimeofday(&et,NULL);
   int elapsed = ((et.tv_sec - st.tv_sec)*1000000)+(et.tv_usec-st.tv_usec);
   printf("Get Attribute took %d seconds\n", elapsed);

   time(&end_time);
   printf("Test 4 End time %s\n",ctime(&end_time));	
}

/**
 * Test to set uid
 * Description: Set uid for file.
 * Strategy:
 *  1. Set uid
 *  2. Verify setting uid by getting attributes
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. uid recieved from getattr should match set uid.
 */
static void set_uid(void **state)
{
   struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);
   struct ut_efs_params *ut_efs_objs = &ut_dir_obj->ut_efs_objs;
   int rc = 0,i;
   int flag = STAT_UID_SET;
   time_t start_time, end_time;
   struct timeval st,et;

   uid_t new_uid = 100;

   time_t cur_time;

   struct stat stat_in, stat_out;
   time(&start_time);
   printf("Test-5 Start time %s\n",ctime(&start_time));	
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_in, 0, sizeof(stat_in));
      stat_in.st_uid = new_uid;

      time(&cur_time);

      ut_efs_objs->file_inode=ut_dir_obj->file_inode[i];
      rc = efs_setattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                      &ut_efs_objs->file_inode, &stat_in, flag);
      ut_assert_int_equal(rc, 0);
   }

   gettimeofday(&st,NULL);
   for(i=0;i<NUM_FILES;i++)
   {
      memset(&stat_out, 0, sizeof(stat_out));
      rc = efs_getattr(ut_efs_objs->efs_fs, &ut_efs_objs->cred,
                      &ut_efs_objs->file_inode, &stat_out);

      ut_assert_int_equal(rc, 0);   
   }
   gettimeofday(&et,NULL);
   int elapsed = ((et.tv_sec - st.tv_sec)*1000000)+(et.tv_usec-st.tv_usec);
   printf("Get Attribute took %d seconds\n", elapsed);
   
   time(&end_time);
   printf("Test-5 End time %s\n",ctime(&end_time));	
}

/**
 * Setup for attr test group
 */
static int attr_test_setup(void **state)
{
   int rc = 0, i = 0;
   struct ut_dir_env *ut_dir_obj = calloc(sizeof(struct ut_dir_env),1);	
   ut_assert_not_null(ut_dir_obj);

   ut_dir_obj->name_list= calloc(sizeof(char*),NUM_FILES);
   if(ut_dir_obj->name_list == NULL){
      rc = -ENOMEM;
      ut_assert_true(0);
   }

   ut_dir_obj->file_inode = calloc(sizeof(unsigned long long),NUM_FILES);
   if(ut_dir_obj->file_inode == NULL){
   rc = -ENOMEM;
   ut_assert_true(0);
   }

   *state = ut_dir_obj;
   rc = ut_efs_fs_setup(state);

   ut_assert_int_equal(rc, 0);

   for(i=0;i<NUM_FILES;i++)
   {
      ut_dir_obj->name_list[i] = malloc(sizeof(char*));
   }

   for(i=0;i<NUM_FILES;i++)
   {
      snprintf(ut_dir_obj->name_list[i],sizeof(char*),"%d",i+1);
      ut_dir_obj->ut_efs_objs.file_inode = 0LL;
      ut_dir_obj->ut_efs_objs.file_name = ut_dir_obj->name_list[i];

      rc = ut_file_create(state);

      ut_dir_obj->file_inode[i] = ut_dir_obj->ut_efs_objs.file_inode;
      ut_assert_int_equal(rc, 0);
   }
   return rc;
}

/**
 * Teardown for attr_ops test group
 */
static int attr_test_teardown(void **state)
{
   int rc = 0, i=0;
   struct ut_dir_env *ut_dir_obj = DIR_ENV_FROM_STATE(state);

   for (i=0;i<NUM_FILES;i++)
   {
      ut_dir_obj->ut_efs_objs.file_name=ut_dir_obj->name_list[i];
      rc = ut_file_delete(state);
      ut_assert_int_equal(rc, 0);
   }

   rc = ut_efs_fs_teardown(state);
   ut_assert_int_equal(rc, 0);

   for(i=0;i<NUM_FILES;i++)
   {
      free(ut_dir_obj->name_list[i]);
   }
   free(ut_dir_obj->file_inode);
   free(*state);

   return rc;
}

int main(void)
{
   int rc = 0;
   char *test_log = "/var/log/eos/test/ut/ut_efs.log";

   printf("Attribute Tests\n");

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
		ut_test_case(set_ctime, NULL, NULL),
		ut_test_case(set_mtime, NULL, NULL),
		ut_test_case(set_atime, NULL, NULL),
		ut_test_case(set_gid, NULL, NULL),
		ut_test_case(set_uid, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, attr_test_setup,
				attr_test_teardown);

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
