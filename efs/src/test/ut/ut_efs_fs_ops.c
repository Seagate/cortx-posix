/*
  Filename: ut_efs_fs.c
 * Description: mplementation tests for efs_fs
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
#include "ut_efs_endpoint_dummy.h"

struct collection_item *cfg_items;

static void test_efs_fs_create(void)
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc = efs_fs_create(&fs_name);

	ut_assert_int_equal(rc, 0);
}

static void test_efs_fs_delete(void)
{
	int rc = 0;
	char *name = "efs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc =  efs_fs_delete(&fs_name);

	ut_assert_int_equal(rc, 0);
}

static int test_efs_cb(const struct efs_fs_list_entry *list,  void *args)
{
	int rc = 0;

	if (args != NULL) {
		rc = -EINVAL;
		goto out;
	}

	printf("CB efs name = %s\n", list->fs_name->s_str);
out:
	return rc;
}

static void test_efs_fs_scan(void)
{
	int rc = 0;

	rc = efs_fs_scan_list(test_efs_cb, (void *)0);
	ut_assert_int_equal(rc, 0);
}

int main(void)
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/ut_efs.log";

	printf("FS Tests\n");

	rc = ut_load_config(CONF_FILE);
	if (rc != 0) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_log = ut_get_config("efs", "log_path", test_log);

	rc = ut_init(test_log);
	if (rc != 0) {
		printf("ut_init failed, log path=%s, rc=%d.\n", test_log, rc);
		exit(1);
	}
	rc = efs_init(EFS_DEFAULT_CONFIG, get_endpoint_dummy_ops());
	if (rc) {
		printf("Failed to intitialize efs\n");
		goto out;
	}

	struct test_case test_list[] = {
		ut_test_case(test_efs_fs_create, NULL, NULL),
		ut_test_case(test_efs_fs_delete, NULL, NULL),
		ut_test_case(test_efs_fs_scan, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	efs_fini();

	ut_fini();

	ut_summary(test_count, test_failed);

out:
	free(test_log);

end:
	return rc;
}
