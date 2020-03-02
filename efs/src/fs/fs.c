/*
 * Filename: fs.c
 * Description: EFS Filesystem functions API.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Satendra Singh <satendra.singh@seagate.com>
*/

#include <errno.h> /* errono */
#include <string.h> /* memcpy */
#include "efs.h" /* efs_tree_create_root */
#include "common.h" /* container_of */
#include "fs.h" /* fs interface */
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "common/log.h" /* logging */

/*data types*/

/*fs object*/
struct efs_fs {
	struct namespace *ns; /*namespace object*/
};

/*fs node : in memory data structure.*/
struct efs_fs_node {
        struct efs_fs efs_fs; /*fs object*/
        LIST_ENTRY(efs_fs_node) link;
};

LIST_HEAD(list, efs_fs_node) fs_list = LIST_HEAD_INITIALIZER();

static int efs_fs_is_empty(const struct efs_fs *fs)
{
	//@todo
	//return -ENOTEMPTY;
	return 0;
}

int efs_fs_lookup(const str256_t *name, struct efs_fs **fs)
{
	int rc = -ENOENT;
	struct efs_fs_node *fs_node;
	str256_t *fs_name = NULL;

	if (fs != NULL) {
		*fs = NULL;
	}

	LIST_FOREACH(fs_node, &fs_list, link) {
		ns_get_name(fs_node->efs_fs.ns, &fs_name);
		if (str256_cmp(name, fs_name) == 0) {
			rc = 0;
			if (fs != NULL) {
				*fs = &fs_node->efs_fs;
			}
			goto out;
		}
	}

out:
	log_debug(STR256_F " rc=%d\n", STR256_P(name), rc);
	return rc;
}

void fs_ns_scan_cb(struct namespace *ns, size_t ns_size)
{
	struct efs_fs_node *fs_node = NULL;

	fs_node = malloc(sizeof(struct efs_fs_node));
	if (fs_node == NULL) {
                log_err("Could not allocate memory for efs_fs_node");
		return;
	}

	fs_node->efs_fs.ns = malloc(ns_size);
	if (fs_node->efs_fs.ns == NULL) {
                log_err("Could not allocate memory for ns object");
		free(fs_node);
		return;
	}

	memcpy(fs_node->efs_fs.ns, ns, ns_size);
	LIST_INSERT_HEAD(&fs_list, fs_node, link);
}

int efs_fs_init(struct collection_item *cfg)
{
	int rc = 0;

	rc = ns_init(cfg);
	if (rc != 0) {
		log_err("efs_fs_init failed");
		goto out;
	}

	rc = ns_scan(fs_ns_scan_cb);

out:
	log_debug("rc=%d\n", rc);
	return rc;
}

int efs_fs_fini()
{
	int rc = 0;
	struct efs_fs_node *fs_node = NULL, *fs_node_ptr = NULL;

	LIST_FOREACH_SAFE(fs_node, &fs_list, link, fs_node_ptr) {
		LIST_REMOVE(fs_node, link);
		free(fs_node->efs_fs.ns);
		free(fs_node);
	}

	rc = ns_fini();

	log_debug("rc=%d", rc);
	return rc;
}

void efs_fs_scan(void (*fs_scan_cb)(const struct efs_fs *))
{
	struct efs_fs_node *fs_node = NULL;

	LIST_FOREACH(fs_node, &fs_list, link) {
		dassert(fs_node != NULL);
		fs_scan_cb(&fs_node->efs_fs);
	}
}

int efs_fs_create(const str256_t *fs_name)
{
        int rc = 0;
	struct namespace *ns;
	struct efs_fs_node *fs_node;
	struct kvs_idx *ns_index;
	size_t ns_size = 0;

	rc = efs_fs_lookup(fs_name, NULL);
        if (rc == 0) {
		log_err(STR256_F " already exist rc=%d\n",
			STR256_P(fs_name), rc);
		rc = -EEXIST;
		goto out;
        }

	/* create new node in fs_list */
	fs_node = malloc(sizeof(struct efs_fs_node));
        if (!fs_node) {
                rc = -ENOMEM;
                log_err("Could not allocate memory for efs_fs_node");
                goto out;
        }
	RC_WRAP_LABEL(rc, free_fs_node, ns_create, fs_name, &ns, &ns_size);

	ns_get_ns_index(ns, &ns_index);
	fs_node->efs_fs.ns = malloc(ns_size);
	memcpy(fs_node->efs_fs.ns, ns, ns_size);

	RC_WRAP_LABEL(rc, out, efs_tree_create_root, ns_index);
        LIST_INSERT_HEAD(&fs_list, fs_node, link);
	goto out;

free_fs_node:
	if (fs_node) {
		free(fs_node);
	}

out:
	log_info("fs_name=" STR256_F " rc=%d", STR256_P(fs_name), rc);
        return rc;
}

int efs_fs_delete(const str256_t *fs_name)
{
	int rc = 0;
	struct efs_fs *fs;
	struct efs_fs_node *fs_node = NULL;
	struct kvs_idx *ns_index;

	rc = efs_fs_lookup(fs_name, &fs);
	if (rc != 0) {
		log_err("Can not delete " STR256_F ". FS doesn't exists.\n",
				STR256_P(fs_name));
		goto out;
	}

	if ((rc=efs_fs_is_empty(fs)) != 0) {
		log_err("Can not delete FS %s. It is not empty");
		goto out;
	}

	/* delete the ns_index */
	ns_get_ns_index(fs->ns, &ns_index);
	RC_WRAP_LABEL(rc, out, efs_tree_delete_root, ns_index);

	/* Remove fs from the efs list */
	fs_node = container_of(fs, struct efs_fs_node, efs_fs);
	LIST_REMOVE(fs_node, link);

	RC_WRAP_LABEL(rc, out, ns_delete, fs->ns);
	fs->ns = NULL;

out:
	log_info("fs_name=" STR256_F " rc=%d", STR256_P(fs_name), rc);
	return rc;
}

void efs_fs_get_name(const struct efs_fs *fs, str256_t **name)
{
	dassert(fs);
	ns_get_name(fs->ns, name);
}
