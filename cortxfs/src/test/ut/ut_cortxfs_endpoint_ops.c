/*
  Filename: cortxfs_endpoint.c
 * Description: implementation tests for cfs_endpoint.
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

#include "ut_cortxfs_endpoint_dummy.h"
#include "ut_cortxfs_endpoint.h"
#include "nsal.h"
#include "internal/fs.h"

struct collection_item *cfg_items;

static void test_cfs_endpoint_create(void)
{
	int rc = 0;
	char *name = "cortxfs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));

	rc = cfs_fs_create(&fs_name);
	ut_assert_int_equal(rc, 0);

	const char *endpoint_options  = "{ \"proto\": \"nfs\", \"mode\": \"rw\", \"secType\": \"sys\", \"Filesystem_id\": \"192.168\", \"client\": \"1\", \"clients\": \"*\", \"Squash\": \"no_root_squash\", \"access_type\": \"RW\", \"protocols\": \"4\" }";

	rc = cfs_endpoint_create(&fs_name, endpoint_options);
	ut_assert_int_equal(rc, 0);
}

static void test_cfs_endpoint_delete(void)
{
	int rc = 0;
	char *name = "cortxfs";
	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));

	rc = cfs_endpoint_delete(&fs_name);
	ut_assert_int_equal(rc, 0);

	rc = cfs_fs_delete(&fs_name);
	ut_assert_int_equal(rc, 0);
}

static int test_fs_cb(const struct cfs_fs_list_entry *list, void *args)
{
	int rc = 0;

	if (list->endpoint_info == NULL) {
		printf("CB cortxfs name %s not exported\n", list->fs_name->s_str);
		rc = -ENOENT;
		goto out;
	} else {
		printf("CB cortxfs name %s exported with %s\n",
		       list->fs_name->s_str, (char *)list->endpoint_info);
	}
out:
	return rc;
}

static void test_cfs_fs_scan(void)
{
	int rc = 0;

	rc = cfs_fs_scan_list(test_fs_cb, (void*)0);
	ut_assert_int_equal(rc, 0);
}

static int test_endpoint_cb(const struct cfs_endpoint_info *list, void *args)
{
	int rc = 0;

	printf("ep_name = %s\n", list->ep_name->s_str);
	printf("ep_id = %d\n", list->ep_id);
	printf("ep_info = %s\n", list->ep_info);

	return rc;
}

static void test_cfs_endpoint_scan(void)
{
	int rc = 0;

	rc = cfs_endpoint_scan(test_endpoint_cb, NULL);
	ut_assert_int_equal(rc, 0);
}

int main(int argc, char *argv[])
{
	int rc = 0;
	char *test_log = "/var/log/cortx/test/ut/ut_cortxfs.logs";

	printf("Endpoint ops Tests\n");
	if (argc > 1) {
		test_log = argv[1];
	}

	rc = ut_init(test_log);
	if (rc != 0) {
		exit(1);
	}

	rc = cfs_init(CFS_DEFAULT_CONFIG, get_endpoint_dummy_ops());
	if (rc) {
		printf("Failed to intitialize cortxfs\n");
		exit(1);
	}

	struct test_case test_list[] = {
		ut_test_case(test_cfs_endpoint_create, NULL, NULL),
		ut_test_case(test_cfs_fs_scan, NULL, NULL),
		ut_test_case(test_cfs_endpoint_scan, NULL, NULL),
		ut_test_case(test_cfs_endpoint_delete, NULL, NULL),
	};

	int test_count = sizeof(test_list)/sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	ut_fini();

	ut_summary(test_count, test_failed);

	return 0;
}
