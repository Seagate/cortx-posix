#include "kvtree.h"
#include "kvnode.h"
#include <string.h>
#include <ut.h>
#include <common/log.h>
#include <errno.h>
#include <debug.h>
#include "namespace.h"

#define DEFAULT_CONFIG "/etc/efs/efs.conf"

struct info {
	char arr[255];
	int uid;
	int gid;
};

static struct collection_item *cfg_items;

struct namespace *ns;

int nsal_start(const char *config_path)
{
	struct collection_item *errors = NULL;
	struct kvstore *kvstore = kvstore_get();
	int rc = 0;

	dassert(kvstore != NULL);

	rc = log_init("/var/log/eos/efs/efs.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		log_debug("Log init failed, rc: %d\n", rc);
		goto out;
	}

	rc = config_from_file("libkvsns", config_path, &cfg_items,
							INI_STOP_ON_ERROR, &errors);
	if (rc) {
		log_err("Can't load config rc = %d", rc);
		rc = -rc;
		goto out;
	}

	rc = kvs_init(kvstore, cfg_items);
	if (rc) {
		log_err("Failed to do kvstore init rc = %d", rc);
		goto out;
	}

out:
	if (rc) {
		free_ini_config_errors(errors);
		return rc;
	}

	log_debug("rc=%d nsal_start done", rc);
	return rc;
}

int ut_kvtree_setup()
{
	int rc = 0;

	rc = ns_init(cfg_items);
	if (rc) {
		log_err("Failed to do ns_init rc = %d", rc);
		goto out;
	}
	/* Creating namespace */
	str256_t ns_name;
	char *name = "eosfs";
	size_t ns_size = 0;
	str256_from_cstr(ns_name, name, strlen(name));
	rc = ns_create(&ns_name, &ns, &ns_size);
	if (rc) {
		log_err("Failed to do ns_create rc = %d", rc);
		goto out;
	}

out:
	return rc;
}

void ut_kvtree_teardown()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	rc = ns_delete(ns);
	if (rc != 0) {
		log_err("ns_delete failed, rc = %d", rc);
	 }

	rc = ns_fini();
	if (rc != 0) {
		log_err("ns_fini failed, rc = %d", rc);
	 }

	rc = kvs_fini(kvstor);
	if (rc != 0) {
		log_err("kvs_fini failed, rc = %d", rc);
	 }
}

void test_kvtree_create()
{
	struct kvtree *kvtree;
	int rc = 0;
	/* Dummy data treated as root information*/
	struct info node_info = {"sample-kvtree",99, 55};

	struct kvnode_info *root_info = NULL;

	rc = kvnode_info_alloc((void *)&node_info, sizeof(struct info), &root_info);
	ut_assert_int_equal(rc, 0);

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, root_info, &kvtree);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(root_info);

}

void test_kvtree_delete()
{
	int rc = 0;
	struct kvtree *kvtree;
	struct info node_info = {"sample-kvtree2",99, 55};

	struct kvnode_info *root_info = NULL;
	rc = kvnode_info_alloc((void *)&node_info, sizeof(struct info), &root_info);
	ut_assert_int_equal(rc, 0);

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, root_info, &kvtree);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(root_info);

	rc = kvtree_delete(kvtree);
	ut_assert_int_equal(rc, 0);

}

int test_internal_node(struct kvtree *kvtree, struct info *test_info);

void test_kvtree_ops()
{
	int rc = 0;
	struct kvtree *kvtree;
	struct info root_test_info = {"sample-kvtree2",99, 55};

	struct kvnode_info *root_info = NULL;
	rc = kvnode_info_alloc((void *)&root_test_info, sizeof(struct info), &root_info);
	ut_assert_int_equal(rc, 0);

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, root_info, &kvtree);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(root_info);

	rc = kvtree_init(ns, kvtree);
	ut_assert_int_equal(rc, 0);

	/*create new kvnode*/
	struct info test_info = {"sample-node1", 1000, 2000};
	struct kvnode_info *node_info = NULL;

	node_id_t node_id = { .f_hi = 33, .f_lo = 0 };

	rc = kvnode_info_alloc((void *)&test_info, sizeof(struct info), &node_info);
	ut_assert_int_equal(rc, 0);

	struct kvnode node;
	rc = kvnode_create(kvtree, node_id, node_info, &node);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(node_info);

	/*check data*/
	rc = test_internal_node(kvtree, &test_info);
	ut_assert_int_equal(rc, 0);

}

int test_internal_node(struct kvtree *kvtree, struct info *test_info)
{
	struct kvnode empty_node;
	int rc = 0;
	node_id_t node_id = { .f_hi = 33, .f_lo = 0 };

	empty_node.kvtree = kvtree;
	empty_node.node_id = node_id;

	rc = kvnode_info_read(&empty_node);
	if (rc) {
		return rc;
	}
	rc = memcmp(test_info, empty_node.node_info->info, empty_node.node_info->size);

	return rc;
}

int main(int argc, char **argv)
{
	int rc = 0;
	char *test_logs = "/var/log/eos/efs/test.logs";

	if (argc > 1) {
		test_logs = argv[1];
	}
	printf("KVTree test\n");

	rc = ut_init(test_logs);

	rc = nsal_start(DEFAULT_CONFIG);
	if (rc) {
		log_err("Failed to do nsal_start rc = %d", rc);
		goto out;
	}
	rc = ut_kvtree_setup();
	if (rc) {
		log_err("Failed to do kvtree_setup rc = %d", rc);
		goto out;
	}
	struct test_case test_list[] = {
		ut_test_case(test_kvtree_create),
		ut_test_case(test_kvtree_delete),
		ut_test_case(test_kvtree_ops)
	};
	int test_count = sizeof(test_list) / sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count);

	ut_kvtree_teardown();
out:
	ut_fini();
	printf("Total tests = %d\n", test_count);
	printf("Tests passed = %d\n", test_count - test_failed);
	printf("Tests failed = %d\n", test_failed);

	return 0;
}
