/*
 * Filename: ut_nsal_kvtree_ops.c 
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
#include "nsal.h"

#define DEFAULT_CONFIG "/etc/cortx/cortxfs.conf"

#define KVNODE_ID_INIT(__id) (node_id_t)   \
{                                          \
    .f_hi = __id,                          \
    .f_lo = 0,                             \
};

/* Dummy structure which acts as a basic attribute for a node*/
#define CONF_FILE "/tmp/eos-fs/build-nsal/test/ut/ut_nsal.conf"
struct info {
	char arr[255];
	int uid;
	int gid;
};

/* Dummy system attribute types for storing sys_attrs along with a kvnode*/
typedef enum sys_attr {
	ATTR_TYPE_1 = 1,
	ATTR_TYPE_2,
	ATTR_TYPE_3
} sys_attr_t;

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

	rc = log_init("/var/log/cortx/fs/efs.log", LEVEL_DEBUG);
	if (rc != 0) {
		rc = -EINVAL;
		printf("Log init failed, rc: %d\n", rc);
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
	}

	log_debug("rc=%d nsal_start done", rc);
	return rc;
}

static int ut_kvtree_setup()
{
	int rc = 0;
	/* Creating namespace */
	str256_t ns_name;
	char *name = "eosfs";
	size_t ns_size = 0;

	rc = nsal_module_init(cfg_items);
	if (rc) {
		log_err("Failed to do ns_module_init rc = %d", rc);
		goto out;
	}

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

	rc = nsal_module_fini();
	if (rc != 0) {
		log_err("ns_module_fini failed, rc = %d", rc);
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

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, (void *)&root_test_info, sizeof(struct info),
	                   &test_kvtree);
	if (rc != 0) {
		printf("kvtree_create failed, rc = %d", rc);
		goto out;
	}

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

	rc = kvtree_fini(test_kvtree);
	if (rc != 0) {
		goto out;
	}

	rc = kvtree_delete(test_kvtree);
	if (rc != 0) {
		goto out;
	}

out:
	return rc;
}

/* Helper function to compare contents of kvnode_attr */
static int ut_internal_node_check(struct kvtree *tree, node_id_t node_id,
                              struct info *test_info)
{
	struct   info *attr_buffer = NULL;
	struct   kvnode empty_node;
	int      rc = 0;
	uint16_t buf_size = 0;

	rc = kvnode_load(tree, &node_id, &empty_node);
	if (rc) {
		goto out;
	}

	buf_size = kvnode_get_basic_attr_buff(&empty_node,
					      (void **)&attr_buffer);

	ut_assert_int_not_equal(buf_size, 0);

	rc = memcmp(test_info, attr_buffer, buf_size);

	kvnode_fini(&empty_node);
out:
	return rc;
}

/**
 * Test to create new (non-existing) kvtree and initialize it.
 * Description: Create new (non-existing) kvtree and initialize it.
 * Strategy:
 *  1. Create kvtree
 *  2. Initialize kvtree
 *  3. Verify root node_basic_attr by loading root node_attr value
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. kvnode_basic_attr recieved from kvnode_load should match written
 *     node_attr at time of root node creation.
 */
static void test_kvtree_create()
{
	struct kvtree *tree;
	int rc = 0;
	/* Dummy data treated as root information*/
	struct info node_info = {"sample-kvtree-root", 99, 55};

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, (void *)&node_info, sizeof(struct info), &tree);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_init(ns, tree);
	ut_assert_int_equal(rc, 0);

	rc = ut_internal_node_check(tree, tree->root_node_id, &node_info);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_fini(tree);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_delete(tree);
	if (rc) {
		printf("Failed to delete kvtree rc = %d\n", rc);
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

	/*passing created namespace to kvtree*/
	rc = kvtree_create(ns, (void *)&node_info, sizeof(struct info), &tree);
	ut_assert_int_equal(rc, 0);

	rc = kvtree_delete(tree);
	ut_assert_int_equal(rc, 0);
}

/**
 * Test to create new (non-existing) kvnode and dump it on disk.
 * Description: Create new (non-existing) kvnode for test_kvtree and dump it.
 * Strategy:
 *  1. Create kvnode
 *  2. Dump kvnode
 *  3. Verify written kvnode basic attributes by kvnode_load value
 * Expected behavior:
 *  1. No errors from EFS API.
 *  2. node_attr recieved from kvnode_load should match written
 *     node_attr at time of node creation.
 */
static void test_kvnode_create()
{
	/*create new kvnode*/
	struct info test_info = {"nonexist-node1", 1000, 2000};
	int rc = 0;
	node_id_t node_id;
	struct kvnode node;

	node_id = KVNODE_ID_INIT(33);
	/* Operation to create kvnode*/
	rc = kvnode_init(test_kvtree, &node_id, (void *)&test_info,
	                 sizeof(struct info), &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&node);

	/*check set data*/
	rc = ut_internal_node_check(test_kvtree, node_id, &test_info);
	ut_assert_int_equal(rc, 0);
}

/* Helper function to create kvnode based on node_id and node_attr
 * and then dump it to the disk. */
static void ut_internal_kvnode_create(struct info *test_info, node_id_t node_id)
{
	int rc = 0;
	struct kvnode node;

	rc = kvnode_init(test_kvtree, &node_id, (void *)test_info,
	                 sizeof(struct info), &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&node);
}

/* Helper function to delete kvnode based on node_id*/
static void ut_internal_kvnode_delete(node_id_t node_id)
{
	int rc = 0;
	struct kvnode node;

	rc = kvnode_load(test_kvtree, &node_id, &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_delete(&node);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&node);
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
	node_id_t node_id;
	/*create new child kvnode*/
	struct info test_info = {"child-node1", 1000, 2000};
	str256_t node_name;
	node_id_t new_id;

	node_id = KVNODE_ID_INIT(44);
	str256_from_cstr(node_name, "child-node1", 11);

	ut_internal_kvnode_create(&test_info, node_id);

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

	new_id = KVNODE_NULL_ID;

	rc = kvtree_lookup(test_kvtree, &(test_kvtree->root_node_id), &node_name,
	                   &new_id);
	ut_assert_int_equal(rc, -ENOENT);

	ut_internal_kvnode_delete(node_id);
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

	rc = kvtree_lookup(test_kvtree, &(test_kvtree->root_node_id), &node_name,
	                   NULL);
	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to dump and load a kvnode.
 * Description: Dump a node and load the same kvnode and verify all content of
 * the fist node matches the content of the second node.
 * Strategy:
 *  1. Dump first kvnode.
 *  2. Load second kvnode from the first node's node_id
 *  3. Delete the origin node from disk.
 * Expected behavior:
 *  1. No errors from kvnode API.
 *  2. All content of the fist node should matche the content of the second node.
 */
static void test_dump_load_verify(void)
{
	int rc = 0;
	struct kvnode origin;
	struct kvnode loaded;
	struct info test_info = {"origin-node1", 1000, 2000};
	node_id_t o_node_id;

	o_node_id = KVNODE_ID_INIT(55);

	rc = kvnode_init(test_kvtree, &o_node_id, &test_info, sizeof(struct info),
	                 &origin);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&origin);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_load(test_kvtree, &origin.node_id, &loaded);
	ut_assert_int_equal(rc, 0);

	/* Deep comparison between all the components of the node*/
	rc = memcmp(&origin.node_id, &loaded.node_id, sizeof(node_id_t));
	ut_assert_int_equal(rc, 0);

	rc = memcmp(origin.basic_attr, loaded.basic_attr,
	            sizeof(struct info));
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&loaded);

	rc = kvnode_delete(&origin);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&origin);
}

/**
 * Test to load a kvnode which does not exist.
 * Description: Load a kvnode which is not present in the test_kvtree.
 * Strategy:
 *  1. Load kvnode from the given node_id
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. Load should return -ENOENT which signifies node didn't exist.
 */
static void test_kvnode_load_nonexist()
{
	int rc = 0;
	node_id_t node_id;
	struct kvnode node;

	node_id = KVNODE_ID_INIT(66);
	/* Operation to delete kvnode */
	rc = kvnode_load(test_kvtree, &node_id, &node);
	ut_assert_int_equal(rc, -ENOENT);
}

/**
 * Test to delete a kvnode which is exists in the kvtree.
 * Description: Delete a kvnode present in the test_kvtree.
 * Strategy:
 *  1. Create a kvnode
 *  2. Delete the above kvnode.
 *  3. Verify the kvnode is deleted or not by kvnode_load using the same
 *     node_id from kvtree.
 * Expected behavior:
 *  1. No errors from kvtree API.
 *  2. After delete, kvnode_load should return -ENOENT which signifies
 *     node was deleted.
 */
static void test_kvnode_delete_exist()
{
	int rc = 0;
	struct kvnode node;
	struct info test_info = {"delete.exist.node", 1000, 2000};
	node_id_t node_id;

	node_id = KVNODE_ID_INIT(77);

	ut_internal_kvnode_create(&test_info, node_id);

	rc = kvnode_load(test_kvtree, &node_id, &node);
	ut_assert_int_equal(rc, 0);

	/* Operation to delete kvnode */
	rc = kvnode_delete(&node);
	ut_assert_int_equal(rc, 0);

	/* check deleted data exists or not*/
	rc = ut_internal_node_check(test_kvtree, node_id, &test_info);
	ut_assert_int_equal(rc, -ENOENT);

	kvnode_fini(&node);
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
static bool test_kvtree_iter_cb(void *ctx,  const char *name,
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
 *     original child node basic attributes.
 */
static void test_kvtree_iter_children()
{
	int rc = 0, i;
	/* Dummy data used for all 5 children. */
	struct info test_info = {"sample.child.node", 1000, 2000};
	/* This node_id is used with f_hi incremented by 1 for each child node.*/
	node_id_t child_id;
	str256_t node_name;

	child_id = KVNODE_ID_INIT(80);
	for (i = 0; i < 5; i++) {
		str256_from_cstr(node_name, children[i], strlen(children[i]));

		ut_internal_kvnode_create(&test_info, child_id);

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

		ut_internal_kvnode_delete(child_id);

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

/**
 * Test to set system attributes of a given numeric-id for a kvnode.
 * Description: Set system attributes of a given numeric-id for a kvnode and
 * verify it using get system attr API.
 * * Strategy:
 * 1. Create a kvnode.
 * 2. SET sys attr for the above kvnode.
 * 3. GET sys attr for the same kvnode.
 * 4. Delete the kvnode.
 * Expected behavior:
 *  1. No errors from kvnode API.
 *  2. Verify that the GET sys_attr API matches the original system attribute
 *     at time of setting it.
 */
static void test_kvnode_set_sys_attr()
{
	int rc = 0;
	struct kvnode node;
	struct info test_info = {"sample.sys_attr.node", 1000, 2000};
	node_id_t node_id;
	char *str = "sample-attr-1";
	buff_t value;
	buff_t loaded;

	node_id = KVNODE_ID_INIT(123);

	rc = kvnode_init(test_kvtree, &node_id, (void *)&test_info,
	                 sizeof(struct info), &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	buff_init(&value, str, strlen(str) + 1);

	rc = kvnode_set_sys_attr(&node, ATTR_TYPE_1, value);
	ut_assert_int_equal(rc, 0);

	buff_init(&loaded, NULL, 0);

	rc = kvnode_get_sys_attr(&node, ATTR_TYPE_1, &loaded);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&node);

	ut_assert_int_equal(loaded.len, value.len);

	rc = memcmp(loaded.buf, value.buf, value.len);
	ut_assert_int_equal(rc, 0);

	free(loaded.buf);

	ut_internal_kvnode_delete(node_id);
}

/**
 * Test to get a non-existing system attributes of a given numeric-id for a kvnode.
 * Description: Get a non-existing system attributes of a given numeric-id for
 * a kvnode and  verify that the API returns -ENOENT.
 * * Strategy:
 * 1. Create a kvnode.
 * 2. GET sys attr for the same kvnode. It should retun -ENOENT.
 * 3. Delete the kvnode.
 * Expected behavior:
 *  1. No errors from kvnode API.
 *  2. Verify that the GET sys_attr API returns -ENOENT for a non-existing
 *     system attribute.
 */
static void test_kvnode_get_sys_attr_nonexist()
{
	int rc = 0;
	struct info test_info = {"sample.non-exist.sys_attr.node", 1000, 2000};
	node_id_t node_id;
	struct kvnode node;
	buff_t value;

	node_id = KVNODE_ID_INIT(124);

	rc = kvnode_init(test_kvtree, &node_id, (void *)&test_info,
	                 sizeof(struct info), &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	buff_init(&value, NULL, 0);

	rc = kvnode_get_sys_attr(&node, ATTR_TYPE_1, &value);
	ut_assert_int_equal(rc, -ENOENT);

	ut_assert_null(value.buf);
	ut_assert_int_equal(value.len, 0);

	rc = kvnode_delete(&node);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&node);
}

/**
 * Test to set multiple system attributes by using multiple numeric-id for a kvnode.
 * Description: Set multiple(list) system attributes for a kvnode and verify
 * the list of set attributes using get system attr API.
 * * Strategy:
 * 1. Create a kvnode.
 * 2. SET list of sys attr for the above kvnode.
 * 3. GET list of sys attr for the same kvnode and verify its contents.
 * 4. Delete all sys attrs.
 * 5. Delete the kvnode.
 * Expected behavior:
 *  1. No errors from kvnode API.
 *  2. Verify that the GET sys_attr API matches the original system attribute
 *     at time of setting it.
 */
static void test_kvnode_set_sys_attr_multiple()
{
	int rc = 0;
	int i;
	struct kvnode node;
	buff_t value;
	struct info test_info = {"sample.sys_attr.node", 1000, 2000};
	node_id_t node_id;
	char *str[] = {
		"sample-attr-type1",
		"sample-attr-type2",
		"sample-attr-type3" };

	node_id = KVNODE_ID_INIT(125);

	rc = kvnode_init(test_kvtree, &node_id, &test_info, sizeof(struct info),
	                 &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	for (i = 0; i < sizeof(str)/sizeof(str[0]); ++i) {
		buff_init(&value, str[i], strlen(str[i]) + 1);

		rc = kvnode_set_sys_attr(&node, ATTR_TYPE_1 + i, value);
		ut_assert_int_equal(rc, 0);
	}

	for (i = 0; i < sizeof(str)/sizeof(str[0]); ++i) {
		buff_init(&value, NULL, 0);

		rc = kvnode_get_sys_attr(&node, ATTR_TYPE_1 + i, &value);
		ut_assert_int_equal(rc, 0);

		ut_assert_not_null(value.buf);
		ut_assert_int_not_equal(value.len, 0);

		rc = memcmp(str[i], value.buf, value.len);
		ut_assert_int_equal(rc, 0);

		rc = kvnode_del_sys_attr(&node, ATTR_TYPE_1 + i);
		ut_assert_int_equal(rc, 0);

		free(value.buf);
	}

	rc = kvnode_delete(&node);
	ut_assert_int_equal(rc, 0);

	kvnode_fini(&node);
}

/**
 * Test to delete system attributes of a given numeric-id for a kvnode.
 * Description: Set and then delete system attributes of a given numeric-id for
 * a kvnode and verify it using get system attr API.
 * * Strategy:
 * 1. Create a kvnode.
 * 2. SET sys attr for the above kvnode.
 * 3. DELETE sys attr for the same kvnode.
 * 4. Verify sys attr has been deleted by calling GET sys attr.
 * 5. Delete the kvnode.
 * Expected behavior:
 * 1. No errors from kvnode API.
 * 2. Verify the deleted sys attr by GET sys_attr API which should return
 *     -ENOENT.
 */
static void test_kvnode_del_sys_attr()
{
	int rc = 0;
	struct kvnode node;
	buff_t value;
	struct info test_info = {"sample-delete.sys_attr.node", 1000, 2000};
	node_id_t node_id;
	char *str = "sample-attr-delete-1";

	node_id = KVNODE_ID_INIT(234);

	rc = kvnode_init(test_kvtree, &node_id, (void *)&test_info,
	                 sizeof(struct info), &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	buff_init(&value, str, strlen(str) + 1);

	rc = kvnode_set_sys_attr(&node, ATTR_TYPE_1, value);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_del_sys_attr(&node, ATTR_TYPE_1);
	ut_assert_int_equal(rc, 0);

	buff_init(&value, NULL, 0);

	rc = kvnode_get_sys_attr(&node, ATTR_TYPE_1, &value);
	ut_assert_int_equal(rc, -ENOENT);

	kvnode_fini(&node);

	ut_internal_kvnode_delete(node_id);
}

/**
 * Test to delete a non-existing system attribute of a given numeric-id for a
 * kvnode.
 * Description: Delete non-existing system attributes of a given numeric-id for
 * a kvnode by deleting same attribute twice and then delete an unknown
 * (non-exist) attribute.
 * * Strategy:
 * 1. Create a kvnode.
 * 2. SET sys attr for the above kvnode.
 * 3. Delete sys attr for the same kvnode.
 * 4. Delete same sys attr twice. Should return -ENOENT.
 * 5. Delete an unknown sys attr. Should return -ENOENT.
 * 6. Delete the kvnode.
 * Expected behavior:
 * 1. No errors from kvnode API.
 * 2. Verify that deleting a non-existing sys attr should return -ENOENT.
 */
static void test_kvnode_del_sys_attr_nonexist()
{
	int rc = 0;
	struct info test_info = {"sample-delete.sys_attr.node", 1000, 2000};
	node_id_t node_id;
	struct kvnode node;
	char *str = "sample-attr-delete-2";
	buff_t value;

	node_id = KVNODE_ID_INIT(345);

	rc = kvnode_init(test_kvtree, &node_id, (void *)&test_info,
	                 sizeof(struct info), &node);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_dump(&node);
	ut_assert_int_equal(rc, 0);

	buff_init(&value, str, strlen(str) + 1);

	rc = kvnode_set_sys_attr(&node, ATTR_TYPE_1, value);
	ut_assert_int_equal(rc, 0);

	rc = kvnode_del_sys_attr(&node, ATTR_TYPE_1);
	ut_assert_int_equal(rc, 0);

	/* Deleting a system attribute twice. */
	rc = kvnode_del_sys_attr(&node, ATTR_TYPE_1);
	ut_assert_int_equal(rc, -ENOENT);

	/* Deleting an unknown type of system attribute. */
	rc = kvnode_del_sys_attr(&node, 898);
	ut_assert_int_equal(rc, -ENOENT);

	kvnode_fini(&node);

	ut_internal_kvnode_delete(node_id);
}

int main(void)
{
	int rc = 0;
	char *test_logs = "/var/log/eos/test/ut/ut_nsal.logs";

	printf("KVTree test\n");

	rc = ut_load_config(CONF_FILE);
	if (rc) {
		printf("ut_load_config: err = %d\n", rc);
		goto end;
	}

	test_logs = ut_get_config("nsal", "log_path", test_logs);

	rc = ut_init(test_logs);
	if (rc) {
		printf("Failed to do ut_init rc = %d", rc);
		goto out;
	}

	rc = nsal_start(DEFAULT_CONFIG);
	if (rc) {
		printf("Failed to do nsal_start rc = %d", rc);
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
		ut_test_case(test_dump_load_verify, NULL, NULL),
		ut_test_case(test_kvnode_load_nonexist, NULL, NULL),
		ut_test_case(test_kvnode_delete_exist, NULL, NULL),
		ut_test_case(test_kvtree_attach_detach, NULL, NULL),
		ut_test_case(test_kvtree_detach_nonexist, NULL, NULL),
		ut_test_case(test_kvtree_lookup_nonexist, NULL, NULL),
		ut_test_case(test_kvtree_iter_children, NULL, NULL),
		ut_test_case(test_kvtree_iter_children_empty, NULL, NULL),
		ut_test_case(test_kvnode_set_sys_attr, NULL, NULL),
		ut_test_case(test_kvnode_get_sys_attr_nonexist, NULL, NULL),
		ut_test_case(test_kvnode_set_sys_attr_multiple, NULL, NULL),
		ut_test_case(test_kvnode_del_sys_attr, NULL, NULL),
		ut_test_case(test_kvnode_del_sys_attr_nonexist, NULL, NULL),
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

	free(test_logs);

end:
	return rc;
}
