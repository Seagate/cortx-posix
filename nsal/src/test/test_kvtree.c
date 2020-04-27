/*
 * Filename: test_kvtree.c
 * Description: Unit tests for kvtree.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Shreya Karmakar <shreya.karmakar@seagate.com>
 */

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

static struct namespace *ns;

/* kvtree pointer to be used in all kvtree ops. */
static struct kvtree *test_kvtree = NULL;

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

static int ut_kvtree_setup()
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

static int ut_kvtree_teardown()
{
	int rc = 0;
	struct kvstore *kvstor = kvstore_get();

	rc = ns_delete(ns);
	if (rc != 0) {
		log_err("ns_delete failed, rc = %d", rc);
		goto out;
	 }

	rc = ns_fini();
	if (rc != 0) {
		log_err("ns_fini failed, rc = %d", rc);
		goto out;
	 }

	rc = kvs_fini(kvstor);
	if (rc != 0) {
		log_err("kvs_fini failed, rc = %d", rc);
		goto out;
	 }
out:
	return rc;
}

/* Creation of test_kvtree for performing kvnode and lookup operations. */
static int ut_kvtree_ops_init()
{
	int rc = 0;
	struct info root_test_info = {"test-kvtree", 99, 55};
	struct kvnode_info *root_info = NULL;

	rc = kvnode_info_alloc((void *)&root_test_info, sizeof(struct info), &root_info);
	ut_assert_int_equal(rc, 0);

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, root_info, &test_kvtree);
	if (rc != 0) {
		printf("kvtree_create failed, rc = %d", rc);
		goto out;
	 }

	kvnode_info_free(root_info);

	rc = kvtree_init(ns, test_kvtree);
	if (rc != 0) {
		printf("kvtree_init failed, rc = %d", rc);
		goto out;
	 }
out:
	return rc;
}

static int ut_kvtree_ops_fini()
{
	int rc = 0;

	rc = kvtree_delete(test_kvtree);
	if (rc != 0) {
		goto out;
	}

	rc = kvtree_fini(test_kvtree);
	if (rc != 0) {
		goto out;
	 }
out:
	return rc;
}

/* Helper function to compare contents of kvnode_info */
static int ut_internal_node_check(struct kvtree *tree, node_id_t node_id,
                              struct info *test_info)
{
	struct kvnode empty_node;
	int rc = 0;

	empty_node.tree = tree;
	empty_node.node_id = node_id;

	rc = kvnode_info_read(&empty_node);
	if (rc) {
		goto out;
	}
	rc = memcmp(test_info, empty_node.node_info->info, empty_node.node_info->size);
out:
	return rc;
}

/**
 * Test to create new (non-existing) kvtree and initialize it.
 * Description: Create new (non-existing) kvtree and initialize it.
 * Strategy:
 *  1. Create kvtree
 *  2. Initialize kvtree
 *  3. Verify root kvnode_info by readding root kvnode_info value
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. kvnode_info recieved from kvnode_info read should match written
 *     kvnode_info at time of root node creation.
 */
static void test_kvtree_create()
{
	struct kvtree *tree;
	int rc = 0;
	/* Dummy data treated as root information*/
	struct info node_info = {"sample-kvtree-root", 99, 55};
	struct kvnode_info *root_info = NULL;

	rc = kvnode_info_alloc((void *)&node_info, sizeof(struct info), &root_info);
	ut_assert_int_equal(rc, 0);

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, root_info, &tree);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(root_info);

	rc = kvtree_init(ns, tree);
	ut_assert_int_equal(rc, 0);

	rc = ut_internal_node_check(tree, tree->root_node_id, &node_info);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_delete(tree);
	if (rc) {
		printf("Failed to delete kvtree \n");
	}
}

/**
 * Test to delete kvtree
 * Description: Delete existing kvtree.
 * Strategy:
 *  1. Create kvtree
 *  2. Initialze kvtree
 *  2. Delete kvtree
 * Expected behavior:
 *  1. No errors from kvtree API.
 */
static void test_kvtree_delete()
{
	int rc = 0;
	struct kvtree *tree;
	struct info node_info = {"sample-kvtree-root2",99, 55};
	struct kvnode_info *root_info = NULL;

	rc = kvnode_info_alloc((void *)&node_info, sizeof(struct info), &root_info);
	ut_assert_int_equal(rc, 0);

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, root_info, &tree);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(root_info);

	rc = kvtree_init(ns, tree);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_delete(tree);
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to create new (non-existing) kvnode
 * Description: Create new (non-existing) kvnode for test_kvtree.
 * Strategy:
 *  1. Create kvnode
 *  2. Verify write kvnode_info by readding kvnode_info value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. kvnode_info recieved from kvnode_info read should match written
 *     kvnode_info at time of node creation.
 */
static void test_kvnode_create()
{
	/*create new kvnode*/
	struct info test_info = {"nonexist-node1", 1000, 2000};
	struct kvnode_info *node_info = NULL;
	int rc = 0;
	node_id_t node_id = { .f_hi = 33, .f_lo = 0 };

	rc = kvnode_info_alloc((void *)&test_info, sizeof(struct info), &node_info);
	ut_assert_int_equal(rc, 0);

	/* Operation to create kvnode*/
	struct kvnode node;
	rc = kvnode_create(test_kvtree, &node_id, node_info, &node);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(node_info);

	/*check set data*/
	rc = ut_internal_node_check(test_kvtree, node_id, &test_info);
	ut_assert_int_equal(rc, 0);
}

/* Helper function to create kvnode based on node_id and node_info*/
static void ut_internal_kvnode_create(struct info *test_info, node_id_t node_id,
                                      struct kvnode *node)
{
	struct kvnode_info *node_info = NULL;
	int rc = 0;

	rc = kvnode_info_alloc((void *)test_info, sizeof(struct info), &node_info);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_create(test_kvtree, &node_id, node_info, node);
	ut_assert_int_equal(rc, 0);

	kvnode_info_free(node_info);
}

/**
 * Test to attach and detach a kvnode to the kvtree.
 * Description: Attach a created kvnode to the parent node present in the
 *              test_kvtree. Then detach the same kvnode from the kvtree.
 * Strategy:
 *  1. Create kvnode
 *  2. Attach kvnode to kvtree -> root node
 *  3. Verify the kvnode is attached or not by doing lookup of the kvnode.
 *  4. Detach kvnode from kvtree.i
 *  5. Verify the kvnode is detached or not by doing lookup of the kvnode.
 *  6. Delete kvnode.
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. Lookup should return the node id which was attached previously.
 *  3. After detach, lookup should return -ENOENT.
 */
static void test_kvtree_attach_detach()
{
	int rc = 0;
	node_id_t node_id = { .f_hi = 33, .f_lo = 0 };
	/*create new child kvnode*/
	struct info test_info = {"child-node1", 1000, 2000};
	struct kvnode node;
	str256_t node_name;
	node_id_t new_id;

	ut_internal_kvnode_create(&test_info, node_id, &node);

	str256_from_cstr(node_name, "child-node1", 11);

	rc = kvtree_attach(test_kvtree, &(test_kvtree->root_node_id), &node_id,
	                   &node_name);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_lookup(test_kvtree, &(test_kvtree->root_node_id), &node_name,
	                   &new_id);
	ut_assert_int_equal(rc, 0);

	rc = memcmp(&node_id, &new_id, sizeof(node_id_t));
	ut_assert_int_equal(rc, 0);

	rc = kvtree_detach(test_kvtree, &(test_kvtree->root_node_id), &node_name);
	ut_assert_int_equal(rc, 0);

	new_id.f_hi = 0, new_id.f_lo = 0;

	rc = kvtree_lookup(test_kvtree, &(test_kvtree->root_node_id), &node_name,
	                   &new_id);
	ut_assert_int_equal(rc, -ENOENT);

	rc = kvnode_delete(test_kvtree, &node_id);
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to detach a kvnode which is not attached to the kvtree.
 * Description: Detach a kvnode from the parent node present in the
 *              test_kvtree.
 * Strategy:
 *  1. Detach kvnode from kvtree
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. Detach should return -ENOENT which signifies node was not attached.
 */
static void test_kvtree_detach_nonexist()
{
	int rc = 0;
	str256_t node_name;

	str256_from_cstr(node_name, "child-node-nonexist", 19);

	rc = kvtree_detach(test_kvtree, &(test_kvtree->root_node_id), &node_name);
	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to lookup a kvnode which is not attached to the kvtree.
 * Description: Lookup a kvnode from the parent node present in the
 *              test_kvtree and node_name for a non-existing node.
 * Strategy:
 *  1. Lookup a kvnode from kvtree
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. Lookup should return -ENOENT which signifies node was not attached.
 */
static void test_kvtree_lookup_nonexist()
{
	int rc = 0;
	str256_t node_name;

	str256_from_cstr(node_name, "child-node-nonexist", 19);

	rc = kvtree_lookup(test_kvtree, &(test_kvtree->root_node_id), &node_name, NULL);
	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to delete a kvnode which does not exist.
 * Description: Delete a kvnode which is not present in the test_kvtree.
 * Strategy:
 *  1. Delete a kvnode from kvtree
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. Delete should return -ENOENT which signifies node didn't exist.
 */
static void test_kvnode_delete_nonexist()
{
	int rc = 0;
	node_id_t node_id = { .f_hi = 66, .f_lo = 0 };

	/* Operation to delete kvnode */
	rc = kvnode_delete(test_kvtree, &node_id);
	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to delete a kvnode which is exists in the kvtree.
 * Description: Delete a kvnode present in the test_kvtree.
 * Strategy:
 *  1. Create a kvnode
 *  2. Delete the above kvnode.
 *  3. Verify the kvnode is deleted or not by kvnode_info_read using the same
 *     node_id from kvtree.
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. After delete, kvnode_info_read should return -ENOENT which signifies
 *     node was deleted.
 */
static void test_kvnode_delete_exist()
{
	int rc = 0;
	struct kvnode node;
	struct info test_info = {"delete.exist.node", 1000, 2000};
	node_id_t node_id = { .f_hi = 77, .f_lo = 0 };

	ut_internal_kvnode_create(&test_info, node_id, &node);

	/* Operation to delete kvnode */
	rc = kvnode_delete(test_kvtree, &node_id);
	ut_assert_int_equal(rc, 0);

	/* check deleted data exists or not*/
	rc = ut_internal_node_check(test_kvtree, node_id, &test_info);
	ut_assert_int_equal(rc, -ENOENT);
}

char *children[] = {
	"Child.node1",
	"Child.node2",
	"Child.node3",
	"Child.node4",
	"Child.node5"
};

int cb_pos = 0;

/* Callback function to be used for kvtree_iter_children*/
static bool test_kvtree_iter_cb(void *ctx, const char *name,
                                const struct kvnode *node)
{
	int rc = 0;

	rc = memcmp(name, children[cb_pos], strlen(children[cb_pos]));
	ut_assert_int_equal(rc, 0);
	cb_pos++;

	return true;
}

/**
 * Test to iterate over all the children of a parent kvnode.
 * Description: Iterate over all the children of a parent kvnode.
 * Strategy:
 *  1. Root node is the parent kvnode.
 *  2. Create the 5 children kvnode.
 *  3. Attach all 5 children kvnode to parent kvnode.
 *  4. Iterate over the parent kvnode children.
 *  5. Verify each child kvnode received through the callback.
 *  6. Detach all the children and delete them.
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. After receiving every child-kvnode, the contents should match with
 *     original child node info.
 */
static void test_kvtree_iter_children()
{
	int rc = 0, i;
	struct kvnode node[5];
	/* Dummy data used for all 5 children. */
	struct info test_info = {"sample.child.node", 1000, 2000};
	/* This node_id is used with f_hi incremented by 1 for each child node.*/
	node_id_t child_id = { .f_hi = 80, .f_lo = 0 };
	str256_t node_name;

	for (i = 0; i < 5; i++) {
		ut_internal_kvnode_create(&test_info, child_id, &node[i]);

		str256_from_cstr(node_name, children[i], strlen(children[i]));
		rc = kvtree_attach(test_kvtree, &test_kvtree->root_node_id, &child_id,
		                   &node_name);
		ut_assert_int_equal(rc, 0);

		child_id.f_hi++;
	}
	rc = kvtree_iter_children(test_kvtree, &test_kvtree->root_node_id,
	                          test_kvtree_iter_cb, NULL);
	ut_assert_int_equal(rc, 0);
	ut_assert_int_equal(cb_pos, 5);

	child_id.f_hi = 80;
	for (i = 0; i < 5; i++) {
		str256_from_cstr(node_name, children[i], strlen(children[i]));
		rc = kvtree_detach(test_kvtree, &test_kvtree->root_node_id, &node_name);
		ut_assert_int_equal(rc, 0);

		rc = kvnode_delete(test_kvtree, &child_id);
		ut_assert_int_equal(rc, 0);

		child_id.f_hi++;
	}
}

/**
 * Test to iterate on a parent kvnode which doesnt have any children.
 * Description: Iterate on a parent kvnode which doesnt have any children.
 * Strategy:
 *  1. Root node is the parent kvnode.
 *  2. Iterate on the parent kvnode.
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. Verify that the kvtree_iter_children API returns no error.
 *  3. Verify that callback hasn't been called.
 */
static void test_kvtree_iter_children_empty()
{
	int rc = 0;
	cb_pos = 0;

	rc = kvtree_iter_children(test_kvtree, &test_kvtree->root_node_id,
	                          test_kvtree_iter_cb, NULL);
	ut_assert_int_equal(rc, 0);
	/* verify that callback hasn't been called. */
	ut_assert_int_equal(cb_pos, 0);
}

int main(int argc, char **argv)
{
	int rc = 0;
	char *test_logs = "/var/log/eos/test/ut/nsal/kvtree_ops.logs";

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

	ut_kvtree_ops_init();

	struct test_case test_list[] = {
		ut_test_case(test_kvnode_create, NULL, NULL),
		ut_test_case(test_kvnode_delete_nonexist, NULL, NULL),
		ut_test_case(test_kvnode_delete_exist, NULL, NULL),
		ut_test_case(test_kvtree_attach_detach, NULL, NULL),
		ut_test_case(test_kvtree_detach_nonexist, NULL, NULL),
		ut_test_case(test_kvtree_lookup_nonexist, NULL, NULL),
		ut_test_case(test_kvtree_iter_children, NULL, NULL),
		ut_test_case(test_kvtree_iter_children_empty, NULL, NULL),
		ut_test_case(test_kvtree_create, NULL, NULL),
		ut_test_case(test_kvtree_delete, NULL, NULL),
	};
	int test_count = sizeof(test_list) / sizeof(test_list[0]);
	int test_failed = 0;

	test_failed = ut_run(test_list, test_count, NULL, NULL);

	ut_kvtree_ops_fini();
	ut_kvtree_teardown();
out:
	ut_fini();

	ut_summary(test_count, test_failed);

	return rc;
}
