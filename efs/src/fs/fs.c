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
#include "common.h" /* container_of */
#include "fs.h" /* fs interface */
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "common/log.h" /* logging */
#include <eos/eos_kvstore.h> /* remove this */
#include "kvtree.h"
#include "kvnode.h"

/*data types*/

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

	fs_node->efs_fs.kvtree = NULL;

	LIST_INSERT_HEAD(&fs_list, fs_node, link);
}

int efs_fs_init(struct collection_item *cfg)
{
	int rc = 0;
	rc = ns_scan(fs_ns_scan_cb);
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

	log_debug("rc=%d", rc);
	return rc;
}

void efs_fs_scan(int (*fs_scan_cb)(const struct efs_fs *, void *args), void *args)
{
	struct efs_fs_node *fs_node = NULL;

	LIST_FOREACH(fs_node, &fs_list, link) {
		dassert(fs_node != NULL);
		fs_scan_cb(&fs_node->efs_fs, args);
	}
}

int efs_fs_create(const str256_t *fs_name)
{
        int rc = 0;
	struct namespace *ns;
	struct efs_fs_node *fs_node;
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

	// Attach ns to efs
	fs_node->efs_fs.ns = malloc(ns_size);
	memcpy(fs_node->efs_fs.ns, ns, ns_size);

	/* @TODO This whole code of index open-close, efs_tree_create_root is 
	 * repetitive and will be removed in second phase of kvtree */ 


	struct kvnode_info *root_node_info = NULL;
	struct kvtree *kvtree = NULL;
	struct stat bufstat;

	/* Set stat */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_mode = S_IFDIR|0777;
	bufstat.st_ino = EFS_ROOT_INODE;
	bufstat.st_nlink = 2;
	bufstat.st_uid = 0;
	bufstat.st_gid = 0;
	bufstat.st_atim.tv_sec = 0;
	bufstat.st_mtim.tv_sec = 0;
	bufstat.st_ctim.tv_sec = 0;
 
	/* intialize root_node_info with efs root stats */
	RC_WRAP_LABEL(rc, free_info, kvnode_info_alloc, (void *)&bufstat, 
	              sizeof(struct stat), &root_node_info);

	RC_WRAP_LABEL(rc, free_info, kvtree_create, ns, root_node_info, &kvtree);

	/* @TODO set inode num generator this FS after deletion of
	 * efs_tree_create_root */

	/* Attach kvtree pointer to efs struct */
	fs_node->efs_fs.kvtree = kvtree;

	rc = efs_tree_create_root(&fs_node->efs_fs);
free_info:
	kvnode_info_free(root_node_info);

	if (rc == 0) {
		LIST_INSERT_HEAD(&fs_list, fs_node, link);
		goto out;
	}

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

	rc = efs_fs_lookup(fs_name, &fs);
	if (rc != 0) {
		log_err("Can not delete " STR256_F ". FS doesn't exists.\n",
				STR256_P(fs_name));
		goto out;
	}

	rc = efs_fs_is_empty(fs);
	if (rc != 0) {
		log_err("Can not delete FS " STR256_F ". It is not empty",
			STR256_P(fs_name));
		goto out;
	}

	RC_WRAP_LABEL(rc, out, efs_tree_delete_root, fs);

	/* delete kvtree */
	RC_WRAP_LABEL(rc, out, kvtree_delete, fs->kvtree);
	fs->kvtree = NULL;

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

int efs_fs_open(const char *fs_name, struct efs_fs **ret_fs)
{
	int rc;
	struct efs_fs *fs = NULL;
	kvs_idx_fid_t ns_fid;
	str256_t name;

	str256_from_cstr(name, fs_name, strlen(fs_name));
	rc = efs_fs_lookup(&name, &fs);
	if (rc != 0) {
		log_err(STR256_F " FS not found rc=%d\n",
				STR256_P(&name), rc);
		rc = -ENOENT;
		goto error;
	}

	ns_get_fid(fs->ns, &ns_fid);
	//RC_WRAP_LABEL(rc, error, kvs_index_open, kvstor, &ns_fid, index);

	if (fs->kvtree == NULL) {
		fs->kvtree = malloc(sizeof(struct kvtree));
		if (!fs->kvtree) {
			rc = -ENOMEM;
			goto out;
		}
	}
	rc = kvtree_init(fs->ns, fs->kvtree);
	*ret_fs = fs;
error:
	if (rc != 0) {
		log_err("Cannot open fid for fs_name=%s, rc:%d", fs_name, rc);
	}
out:
	return rc;
}

void efs_fs_close(struct efs_fs *efs_fs)
{
	kvtree_fini(efs_fs->kvtree);

}
