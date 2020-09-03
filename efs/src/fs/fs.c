/*
 * Filename: fs.c
 * Description: EFS Filesystem functions API.
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

#include <errno.h> /* errono */
#include <string.h> /* memcpy */
#include "common.h" /* container_of */
#include "internal/fs.h" /* fs interface */
#include "namespace.h" /* namespace methods */
#include "tenant.h" /* tenant method */
#include "common/helpers.h" /* RC_WRAP_LABEL */
#include "common/log.h" /* logging */
#include "kvtree.h"
#include "kvnode.h"

/* data types */
/* fs node : in memory data structure. */
struct efs_fs_node {
	struct efs_fs efs_fs; /* fs object */
	LIST_ENTRY(efs_fs_node) link;
};

LIST_HEAD(list, efs_fs_node) fs_list = LIST_HEAD_INITIALIZER();

/* global endpoint operations for efs*/
static const struct efs_endpoint_ops *g_e_ops;

static int efs_fs_is_empty(const struct efs_fs *fs)
{
	//@todo
	//return -ENOTEMPTY;
	return 0;
}

void efs_fs_get_id(struct efs_fs *fs, uint16_t *fs_id)
{
	dassert(fs);
	ns_get_id(fs->ns, fs_id);
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
	if (rc == 0) {
		/**
		 * Any incore struct efs_fs entry found in fs_list must have
		 * kvtree for the root node attached.
		 * If not, then it is a bug!
		 **/
		dassert(fs_node->efs_fs.kvtree != NULL);
	}

	log_debug(STR256_F " rc=%d", STR256_P(name), rc);
	return rc;
}

void fs_ns_scan_cb(struct namespace *ns, size_t ns_size)
{
	str256_t *fs_name = NULL;
	struct efs_fs_node *fs_node = NULL;

	/**
	 * Before this fs can be added to the incore list fs_list, we must
	 * prepare the incore structure to be usable by others, i.e.
	 * the kvtree and namespace should be attached.
	 */
	ns_get_name(ns, &fs_name);
	log_info("%d trying to load FS: " STR256_F,
		 __LINE__, STR256_P(fs_name));

	fs_node = calloc(1, sizeof(struct efs_fs_node));
	if (fs_node == NULL) {
	    log_err("%d failed to load FS: " STR256_F
		    " , could not allocate memory for efs_fs_node!",
		    __LINE__, STR256_P(fs_name));
	    goto out_failed;
	}

	fs_node->efs_fs.ns = calloc(1, ns_size);
	if (fs_node->efs_fs.ns == NULL) {
	    log_err("%d failed to load FS: " STR256_F
		    " , could not allocate memory for ns object!",
		    __LINE__, STR256_P(fs_name));
	    goto out_failed;
	}

	memcpy(fs_node->efs_fs.ns, ns, ns_size);

	fs_node->efs_fs.kvtree = calloc(1, sizeof(struct kvtree));
	if (!fs_node->efs_fs.kvtree) {
	    log_err("%d failed to load FS: " STR256_F
		    " , could not allocate memory for kvtree object!",
		    __LINE__, STR256_P(fs_name));
	    goto out_failed;
	}

	if (kvtree_init(fs_node->efs_fs.ns, fs_node->efs_fs.kvtree) != 0) {
	    log_err("%d failed to load FS: " STR256_F
		    " , kvtree_init() failed!", __LINE__,
		    STR256_P(fs_name));
	    goto out_failed;
	}

	fs_node->efs_fs.root_node = malloc(sizeof(struct kvnode));
	if (!fs_node->efs_fs.root_node) {
		log_err("%d failed to load FS: " STR256_F
			" , could not allocate memory for kvnode object!",
			__LINE__, STR256_P(fs_name));
		goto out_failed;
	}

	*(fs_node->efs_fs.root_node) = KVNODE_INIT_EMTPY;

	if (kvnode_load(fs_node->efs_fs.kvtree,
			&(fs_node->efs_fs.kvtree->root_node_id),
			fs_node->efs_fs.root_node) != 0) {
            log_err("%d failed to load FS: " STR256_F
                    " , kvnode_load() failed!", __LINE__,
                    STR256_P(fs_name));
            goto out_failed;
        }

	LIST_INSERT_HEAD(&fs_list, fs_node, link);
	log_info("FS:" STR256_F " loaded from disk, ptr:%p",
		 STR256_P(fs_name), &fs_node->efs_fs);
	return;

out_failed:

	log_info("FS:" STR256_F " failed to load from disk",
		 STR256_P(fs_name));

	if (fs_node) {
		if (fs_node->efs_fs.ns) {
			free(fs_node->efs_fs.ns);
		}
		if (fs_node->efs_fs.root_node) {
			/* kvnode_fini is a no-op if root_node is not loaded */
			kvnode_fini(fs_node->efs_fs.root_node);
			free(fs_node->efs_fs.root_node);
		}
		if (fs_node->efs_fs.kvtree) {
			kvtree_fini(fs_node->efs_fs.kvtree);
			free(fs_node->efs_fs.kvtree);
		}

		free(fs_node);
	}
}

static int endpoint_tenant_scan_cb(void *cb_ctx, struct tenant *tenant)
{
	int rc = 0;
	str256_t *endpoint_name = NULL;
	struct efs_fs *fs = NULL;
	struct efs_fs_node *fs_node = NULL;

	if (tenant == NULL) {
		rc = -ENOENT;
		goto out;
	}

	tenant_get_name(tenant, &endpoint_name);

	rc = efs_fs_lookup(endpoint_name, &fs);
	log_debug("FS for tenant " STR256_F " is %p, rc = %d",
		  STR256_P(endpoint_name), fs, rc);

	/* TODO: We don't have auto-recovery for such cases,
	* so that we just halt execution if we cannot find
	* the corresponding filesystem. Later on,
	* EFS should make an attempt to recover the filesystem,
	* and report to the user (using some alert mechanism
	* that can be implemented in CSM) in case if recovery
	* is not possible.
	*/
	if (rc != 0) {
		log_err("Tenant list and FS list is not consistent %d.", rc);
	}
	dassert(rc == 0);

	/* update fs_list */
	fs_node = container_of(fs, struct efs_fs_node, efs_fs);
	RC_WRAP_LABEL(rc, out, tenant_copy, tenant, &fs_node->efs_fs.tenant);
out:
	return rc;
}

int efs_fs_init(const struct efs_endpoint_ops *e_ops)
{
	int rc = 0;
	/* initailize the efs module endpoint operation */
	dassert(e_ops != NULL);
	g_e_ops = e_ops;
	RC_WRAP_LABEL(rc, out, ns_scan, fs_ns_scan_cb);
	RC_WRAP_LABEL(rc, out, efs_endpoint_init);
out:
	log_debug("filesystem initialization, rc=%d", rc);
	return rc;
}

int efs_endpoint_init(void)
{
	int rc = 0;

	RC_WRAP_LABEL(rc, out, tenant_scan, endpoint_tenant_scan_cb, NULL);
	dassert(g_e_ops->init != NULL);
	RC_WRAP_LABEL(rc, out, g_e_ops->init);
out:
	log_debug("endpoint initialization, rc=%d", rc);
	return rc;
}

int efs_endpoint_fini(void)
{
	int rc = 0;
	struct efs_fs_node *fs_node = NULL, *fs_node_ptr = NULL;

	dassert(g_e_ops->fini != NULL);
	RC_WRAP_LABEL(rc, out, g_e_ops->fini);
	LIST_FOREACH_SAFE(fs_node, &fs_list, link, fs_node_ptr) {
		fs_node->efs_fs.tenant = NULL;
	}

out:
	log_debug("endpoint finalize, rc=%d", rc);
	return rc;
}

int efs_fs_fini(void)
{
	int rc = 0;
	struct efs_fs_node *fs_node = NULL, *fs_node_ptr = NULL;

	RC_WRAP_LABEL(rc, out, efs_endpoint_fini);
	LIST_FOREACH_SAFE(fs_node, &fs_list, link, fs_node_ptr) {
		LIST_REMOVE(fs_node, link);
		tenant_free(fs_node->efs_fs.tenant);
		kvnode_fini(fs_node->efs_fs.root_node);
		kvtree_fini(fs_node->efs_fs.kvtree);
	
		free(fs_node->efs_fs.ns);
		free(fs_node->efs_fs.root_node);
		free(fs_node->efs_fs.kvtree);
		free(fs_node);
	}

out:
	log_debug("filesystem finalize, rc=%d", rc);
	return rc;
}

int efs_fs_scan_list(int (*fs_scan_cb)(const struct efs_fs_list_entry *list,
		     void *args), void *args)
{
	int rc = 0;
	struct efs_fs_node *fs_node = NULL;
	struct efs_fs_list_entry fs_entry;
	LIST_FOREACH(fs_node, &fs_list, link) {
		dassert(fs_node != NULL);
		dassert(fs_node->efs_fs.ns != NULL);
		efs_fs_get_name(&fs_node->efs_fs, &fs_entry.fs_name);
		efs_fs_get_endpoint(&fs_node->efs_fs,
				     (void **)&fs_entry.endpoint_info);
		RC_WRAP_LABEL(rc, out, fs_scan_cb, &fs_entry, args);
	}
out:
	return rc;
}

int efs_endpoint_scan(int (*efs_scan_cb)(const struct efs_endpoint_info *info,
                     void *args), void *args)
{
	int rc = 0;
	struct efs_fs_node *fs_node = NULL;
	struct efs_endpoint_info ep_list;

	LIST_FOREACH(fs_node, &fs_list, link) {
		dassert(fs_node != NULL);
		dassert(fs_node->efs_fs.ns != NULL);

		if (fs_node->efs_fs.tenant == NULL)
			continue;

		efs_fs_get_name(&fs_node->efs_fs, &ep_list.ep_name);
		efs_fs_get_id(&fs_node->efs_fs, &ep_list.ep_id);
		efs_fs_get_endpoint(&fs_node->efs_fs,
				     (void **)&ep_list.ep_info);
		RC_WRAP_LABEL(rc, out, efs_scan_cb, &ep_list, args);
	}
out:
	return rc;
}

int efs_fs_create(const str256_t *fs_name)
{
        int rc = 0;
	struct namespace *ns;
	struct efs_fs_node *fs_node;
	size_t ns_size = 0;
	struct kvnode *root_node = NULL;

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
	fs_node->efs_fs.tenant = NULL;

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

	RC_WRAP_LABEL(rc, free_info, kvtree_create, ns, (void *)&bufstat,
	              sizeof(struct stat), &kvtree);

	/* Attach kvtree pointer to efs struct */
	fs_node->efs_fs.kvtree = kvtree;

	RC_WRAP_LABEL(rc, free_fs_node, kvtree_init, ns,
		      fs_node->efs_fs.kvtree);

	root_node = malloc(sizeof(struct kvnode));
	if (!root_node) {
		rc = -ENOMEM;
		log_err("Could not allocate memory for root kvnode");
		goto free_fs_node;
	}

	*root_node = KVNODE_INIT_EMTPY;
	RC_WRAP_LABEL(rc, free_fs_node, kvnode_load, kvtree, &(kvtree->root_node_id),
		      root_node);
	fs_node->efs_fs.root_node = root_node;

	RC_WRAP_LABEL(rc, free_fs_node, efs_ino_num_gen_init,
		      &fs_node->efs_fs);

free_info:

	if (rc == 0) {
		LIST_INSERT_HEAD(&fs_list, fs_node, link);
		goto out;
	}

free_fs_node:
	if (fs_node) {
		if (fs_node->efs_fs.ns)
		{
			free(fs_node->efs_fs.ns);
		}
		if (root_node)
		{
			kvnode_fini(fs_node->efs_fs.root_node);
			free(fs_node->efs_fs.root_node);
		}
		if (kvtree)
		{
			kvtree_fini(fs_node->efs_fs.kvtree);
			free(fs_node->efs_fs.kvtree);
		}

		free(fs_node);
	}

out:
	log_info("fs_name=" STR256_F " rc=%d", STR256_P(fs_name), rc);
        return rc;
}

int efs_endpoint_create(const str256_t *endpoint_name, const char *endpoint_options)
{
	int rc = 0;
	uint16_t fs_id = 0;
	struct efs_fs_node *fs_node = NULL;
	struct efs_fs *fs = NULL;
	struct tenant *tenant;


	/* check file system exist */
	rc = efs_fs_lookup(endpoint_name, &fs);
	if (rc != 0) {
		log_err("Can't create endpoint for non existent fs");
		rc = -ENOENT;
		goto out;
	}

	if (fs->tenant != NULL ) {
		log_err("fs=" STR256_F " already exported",
			 STR256_P(endpoint_name));
		rc = -EEXIST;
		goto out;
	}

	/* get filesyetm ID */
	efs_fs_get_id(fs, &fs_id);

	dassert(g_e_ops->create != NULL);
	RC_WRAP_LABEL(rc, out, g_e_ops->create, endpoint_name->s_str,
		      fs_id,endpoint_options);
	/* create tenant object */
	RC_WRAP_LABEL(rc, out, tenant_create, endpoint_name, &tenant,
		      fs_id, endpoint_options);

	/* update fs_list */
	fs_node = container_of(fs, struct efs_fs_node, efs_fs);
	RC_WRAP_LABEL(rc, out, tenant_copy, tenant, &fs_node->efs_fs.tenant);
	tenant = NULL;

out:
	log_info("endpoint_name=" STR256_F " rc=%d", STR256_P(endpoint_name), rc);
	return rc;
}

int efs_endpoint_delete(const str256_t *endpoint_name)
{
	int rc = 0;
	uint16_t ns_id = 0;
	struct efs_fs *fs = NULL;
	struct efs_fs_node *fs_node = NULL;

	/* check endpoint exist */
	rc = efs_fs_lookup(endpoint_name, &fs);
	if (rc != 0) {
		log_err("Can not delete " STR256_F ". endpoint for non existent. fs",
			 STR256_P(endpoint_name));
		rc = -ENOENT;
		goto out;
	}

	if (fs->tenant == NULL) {
		log_err("Can not delete " STR256_F ". endpoint. Doesn't existent.",
			 STR256_P(endpoint_name));
		rc = -ENOENT;
		goto out;
	}

	/** TODO: check if FS is mounted.
	 * We should not remove the endpoint it is still
	 * "mounted" on one of the clients. Right now,
	 * we don't have a mechanism to check that,
	 * so that ignore this requirement.
	 */

	/* get namespace ID */
	efs_fs_get_id(fs, &ns_id);

	dassert(g_e_ops->delete != NULL);
	RC_WRAP_LABEL(rc, out, g_e_ops->delete, ns_id);
	/* delete tenant from nsal */
	fs_node = container_of(fs, struct efs_fs_node, efs_fs);
	RC_WRAP_LABEL(rc, out, tenant_delete, fs_node->efs_fs.tenant);

	/* Remove endpoint from the fs list */
	tenant_free(fs_node->efs_fs.tenant);
	fs_node->efs_fs.tenant= NULL;

out:
	log_info("endpoint_name=" STR256_F " rc=%d", STR256_P(endpoint_name), rc);
	return rc;
}

int efs_fs_delete(const str256_t *fs_name)
{
	int rc = 0;
	struct efs_fs *fs;
	struct efs_fs_node *fs_node = NULL;

	rc = efs_fs_lookup(fs_name, &fs);
	if (rc != 0) {
		log_err("Can not delete " STR256_F ". FS doesn't exists.",
			STR256_P(fs_name));
		goto out;
	}

	if (fs->tenant != NULL) {
		log_err("Can not delete exported " STR256_F ". Fs",
			STR256_P(fs_name));
		rc = -EINVAL;
		goto out;
	}

	rc = efs_fs_is_empty(fs);
	if (rc != 0) {
		log_err("Can not delete FS " STR256_F ". It is not empty",
			STR256_P(fs_name));
		goto out;
	}

	RC_WRAP_LABEL(rc, out, efs_ino_num_gen_fini, fs);
	RC_WRAP_LABEL(rc, out, kvtree_fini, fs->kvtree);

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

void efs_fs_get_endpoint(const struct efs_fs *fs, void **info)
{
	dassert(fs);
	if (fs->tenant == NULL) {
		*info = NULL;
	} else {
		tenant_get_info(fs->tenant, info);
	}
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
		log_err(STR256_F " FS not found rc=%d",
				STR256_P(&name), rc);
		rc = -ENOENT;
		goto error;
	}

	ns_get_fid(fs->ns, &ns_fid);
	//RC_WRAP_LABEL(rc, error, kvs_index_open, kvstor, &ns_fid, index);

	*ret_fs = fs;

error:
	log_info("efs_fs_open done, FS: %s, rc: %d, ptr: %p",
		 fs_name, rc, fs);
	if (rc != 0) {
		log_err("Cannot open fid for fs_name=%s, rc:%d", fs_name, rc);
	}

	return rc;
}

void efs_fs_close(struct efs_fs *efs_fs)
{
	// This is empty placeholder for future use
	log_warn("Unused function is being called!");
}
