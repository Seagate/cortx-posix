/*
 * Filename: test_efs_fs.c
 * Description: mplementation tests for efs_fs
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 */

#include "test_efs.h"

struct collection_item *cfg_items;

void test_efs_fs_create()
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc = efs_fs_create(&fs_name);

	ut_assert_int_equal(rc, 0);
}

void test_efs_fs_delete()
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc =  efs_fs_delete(&fs_name);

	ut_assert_int_equal(rc, 0);
}

void test_efs_fs_init()
{
        int rc = 0;
        rc = efs_fs_init(cfg_items);

	ut_assert_int_equal(rc, 0);
}

void test_efs_cb(const struct efs_fs *fs)
{
	str256_t *fs_name = NULL;
	efs_fs_get_name(fs, &fs_name);

	printf("CB efs name = %s\n", fs_name->s_str);
}

void test_efs_fs_scan()
{
	efs_fs_scan(test_efs_cb);
}

void test_efs_fs_fini()
{
	int rc = 0;

	rc = efs_fs_fini();

	ut_assert_int_equal(rc, 0);
}
