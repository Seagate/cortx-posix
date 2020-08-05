/*
 * Filename:         efs_ops.c
 * Description:      EOS file system operations
 * This file contains EFS file system operations
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

#include <common/log.h> /* log_debug() */
#include <kvstore.h> /* struct kvstore */
#include <efs.h> /* efs_get_stat() */
#include <debug.h> /* dassert() */
#include <common/helpers.h> /* RC_WRAP_LABEL() */
#include <limits.h> /* PATH_MAX */
#include <string.h> /* memcpy() */
#include <sys/time.h> /* gettimeofday() */
#include <errno.h>  /* errno, -EINVAL */
#include <efs_fh.h> /* efs_fh */
#include "efs_internal.h" /* dstore_obj_delete() */
#include <common.h> /* likely */
#include "kvtree.h"

/* Internal efs structure which holds the information given
 * by upper layer in case of readdir operation
 */
struct efs_readdir_ctx {
	efs_readdir_cb_t cb;
	void *ctx;
};

int efs_getattr(struct efs_fs *efs_fs, const efs_cred_t *cred,
		const efs_ino_t *ino, struct stat *bufstat)
{
	int rc;
	struct stat *stat = NULL;
	struct kvstore *kvstor = kvstore_get();
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(cred != NULL);
	dassert(ino != NULL);
	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, efs_kvnode_load, &node, efs_fs->kvtree, ino);
	RC_WRAP_LABEL(rc, out, efs_get_stat, &node, &stat);
	memcpy(bufstat, stat, sizeof(struct stat));
	kvs_free(kvstor, stat);
out:
	kvnode_fini(&node);
	log_debug("ino=%d rc=%d", (int)bufstat->st_ino, rc);
	return rc;
}

int efs_setattr(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *ino,
		struct stat *setstat, int statflag)
{
	struct stat bufstat;
	struct kvnode node = KVNODE_INIT_EMTPY;
	struct timeval t;
	mode_t ifmt;
	int rc;

	dassert(cred != NULL);
	dassert(setstat != NULL);
	dassert(ino != NULL);

	if (statflag == 0) {
		rc = 0;
		/* Nothing to do */
		goto out;
	}

	rc = gettimeofday(&t, NULL);
	dassert(rc == 0);

	RC_WRAP_LABEL(rc, out, efs_getattr, efs_fs, cred, ino, &bufstat);
	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, cred, ino, EFS_ACCESS_SETATTR);
	/* ctime is to be updated if md are changed */
	bufstat.st_ctim.tv_sec = t.tv_sec;
	bufstat.st_ctim.tv_nsec = 1000 * t.tv_usec;

	if (statflag & STAT_MODE_SET) {
		ifmt = bufstat.st_mode & S_IFMT;
		bufstat.st_mode = setstat->st_mode | ifmt;
	}

	if (statflag & STAT_UID_SET) {
		bufstat.st_uid = setstat->st_uid;
	}

	if (statflag & STAT_GID_SET) {
		bufstat.st_gid = setstat->st_gid;
	}

	if (statflag & STAT_SIZE_SET) {
		bufstat.st_size = setstat->st_size;
		bufstat.st_blocks = setstat->st_blocks;
	}

	if (statflag & STAT_SIZE_ATTACH) {
		dassert(0); /* Unsupported */
	}

	if (statflag & STAT_ATIME_SET) {
		bufstat.st_atim.tv_sec = setstat->st_atim.tv_sec;
		bufstat.st_atim.tv_nsec = setstat->st_atim.tv_nsec;
	}

	if (statflag & STAT_MTIME_SET) {
		bufstat.st_mtim.tv_sec = setstat->st_mtim.tv_sec;
		bufstat.st_mtim.tv_nsec = setstat->st_mtim.tv_nsec;
	}

	if (statflag & STAT_CTIME_SET) {
		bufstat.st_ctim.tv_sec = setstat->st_ctim.tv_sec;
		bufstat.st_ctim.tv_nsec = setstat->st_ctim.tv_nsec;
	}

	RC_WRAP_LABEL(rc, out, efs_kvnode_init, &node, efs_fs->kvtree, ino,
		      &bufstat);
	RC_WRAP_LABEL(rc, out, efs_set_stat, &node);
out:
	kvnode_fini(&node);
	log_debug("rc=%d", rc);
	return rc;
}

int efs_access(struct efs_fs *efs_fs, const efs_cred_t *cred,
	       const efs_ino_t *ino, int flags)
{
	int rc = 0;
	struct stat stat;

	dassert(cred && ino);

	RC_WRAP_LABEL(rc, out, efs_getattr, efs_fs, cred, ino, &stat);
	RC_WRAP_LABEL(rc, out, efs_access_check, cred, &stat, flags);
out:
	return rc;
}

bool efs_readdir_cb(void *cb_ctx, const char *name, const struct kvnode *node)
{
	bool retval = false;
	int  errcode;
	struct stat *child_stat = NULL;
	struct kvstore *kvstor = kvstore_get();
	struct efs_readdir_ctx *cb_info = cb_ctx;

	errcode = efs_get_stat(node, &child_stat);
	/* This error code is not propogated to caller as it expect true/false
	 * and based on that it take the decision to further process the child,
	 * so the error condtion should be asserted here.
	 */
	dassert(errcode == 0);

	retval = cb_info->cb(cb_info->ctx, name, child_stat);

	kvs_free(kvstor, child_stat);

	log_trace("efs_readdir_cb:" NODE_ID_F ", errcode=%d, retVal = %d",
		  NODE_ID_P(&node->node_id), errcode, (int)retval);

	return retval;
}

int efs_readdir(struct efs_fs *efs_fs,
		const efs_cred_t *cred,
		const efs_ino_t *dir_ino,
		efs_readdir_cb_t cb,
		void *cb_ctx)
{
	int rc;
	struct efs_readdir_ctx cb_info = { .cb = cb, .ctx = cb_ctx};
	struct kvnode node = KVNODE_INIT_EMTPY;

	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, (efs_cred_t *)cred,
                      (efs_ino_t *)dir_ino, EFS_ACCESS_LIST_DIR);

	RC_WRAP_LABEL(rc, out, efs_kvnode_load, &node, efs_fs->kvtree, dir_ino);

	RC_WRAP_LABEL(rc, out, kvtree_iter_children, efs_fs->kvtree,
		      &node.node_id, efs_readdir_cb, &cb_info);

	RC_WRAP_LABEL(rc, out, efs_update_stat, &node, STAT_ATIME_SET);

out:
	kvnode_fini(&node);
	return rc;
}

int efs_mkdir(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *parent, char *name,
	      mode_t mode, efs_ino_t *newdir)
{
	int rc;
	dstore_oid_t oid;
	struct dstore *dstore = dstore_get();

	log_trace("ENTER: parent=%p name=%s dir=%p mode=0x%X",
		  parent, name, newdir, mode);

	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, cred, parent,
		      EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_create_entry, efs_fs, cred, parent, name,
		      NULL, mode, newdir, EFS_FT_DIR);

	/* Get a new unique oid */
	RC_WRAP_LABEL(rc, out, dstore_get_new_objid, dstore, &oid);

	/* Set the ino-oid mapping for this directory in kvs.*/
	RC_WRAP_LABEL(rc, out, efs_set_ino_oid, efs_fs, newdir, &oid);

out:
	log_trace("EXIT: parent=%p name=%s dir=%p mode=0x%X rc=%d",
		   parent, name, newdir, mode, rc);
	return rc;
}

int efs_lookup(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *parent,
	       char *name, efs_ino_t *ino)

{
	/* Porting notes:
	 * This call is used by EFS in many places so that it cannot be
	 * direcly replaced by efs_fh_lookup
	 * without modifying them.
	 * However, we should not have multiple versions of the same
	 * file operation. There should be only one implementation
	 * of lookup to make sure that we are testing/debugging
	 * the right thing but not some old/deprecated thing.
	 * So that, this function is modified to use efs_fh
	 * internally but it preserves the old interface.
	 * Eventually the other code will slowly migrate to kvsns_fh_lookup
	 * and this code will be dissolved.
	 */
	int rc;
	struct efs_fh *parent_fh = NULL;
	struct efs_fh *fh = NULL;

	RC_WRAP_LABEL(rc, out, efs_fh_from_ino, efs_fs, parent, NULL, &parent_fh);
	RC_WRAP_LABEL(rc, out, efs_fh_lookup, cred, parent_fh, name, &fh);

	*ino = *efs_fh_ino(fh);

out:
	efs_fh_destroy(parent_fh);
	efs_fh_destroy(fh);
	return rc;
}

int efs_readlink(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *lnk,
		 char *content, size_t *size)
{
	int rc;
	struct kvnode node = KVNODE_INIT_EMTPY;
	struct kvstore *kvstor = kvstore_get();
	buff_t value;

	log_trace("ENTER: symlink_ino=%llu", *lnk);
	dassert(cred && lnk && size);
	dassert(*size != 0);

	buff_init(&value, NULL, 0);

	RC_WRAP_LABEL(rc, errfree, efs_kvnode_load, &node, efs_fs->kvtree, lnk);
	RC_WRAP_LABEL(rc, errfree, efs_update_stat, &node, STAT_ATIME_SET);

	/* Get symlink attributes */
	RC_WRAP_LABEL(rc, errfree, efs_get_sysattr, &node, &value,
		      EFS_SYS_ATTR_SYMLINK);

	dassert(value.len <= PATH_MAX);

	if (value.len > *size) {
		rc = -ENOBUFS;
		goto errfree;
	}

	memcpy(content, value.buf, value.len);
	*size = value.len;

	log_debug("Got link: content='%.*s'", (int) *size, content);

errfree:
	kvnode_fini(&node);

	if (value.buf) {
		kvs_free(kvstor, value.buf);
	}

	log_trace("efs_readlink: rc=%d", rc);
	return rc;
}
/** Default mode for a symlink object.
 * Here is a quote from `man 7 symlink`:
 *        On Linux, the permissions of a symbolic link are not used in any
 *        operations; the permissions are always 0777 (read, write, and execute
 *        for all user categories), and can't be changed.
 */
#define EFS_SYMLINK_MODE 0777

int efs_symlink(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *parent,
		char *name, char *content, efs_ino_t *newlnk)
{
	int rc;
	struct kvnode node = KVNODE_INIT_EMTPY;

	log_trace("ENTER: name=%s", name);
	dassert(cred && parent && name && content && newlnk);

	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, cred, parent, EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_create_entry, efs_fs, cred, parent, name, content,
		      EFS_SYMLINK_MODE, newlnk, EFS_FT_SYMLINK);

	RC_WRAP_LABEL(rc, out, efs_kvnode_load, &node, efs_fs->kvtree, parent);
	RC_WRAP_LABEL(rc, out, efs_update_stat, &node,
		      STAT_MTIME_SET|STAT_CTIME_SET);

out:
	kvnode_fini(&node);
	log_trace("name=%s content=%s rc=%d", name, content, rc);
	return rc;
}


int efs_link(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *ino,
	     efs_ino_t *dino, char *dname)
{
	int rc;
	efs_ino_t tmpino = 0LL;
	str256_t k_name;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(cred && ino && dname && dino && kvstor);

	index = efs_fs->kvtree->index;

	log_trace("ENTER: ino=%llu dino=%llu dname=%s", *ino, *dino, dname);
	RC_WRAP(kvs_begin_transaction, kvstor, &index);
	RC_WRAP_LABEL(rc, aborted, efs_access, efs_fs, cred, dino, EFS_ACCESS_WRITE);

	rc = efs_lookup(efs_fs, cred, dino, dname, &tmpino);
	if (rc == 0)
		return -EEXIST;

	str256_from_cstr(k_name, dname, strlen(dname));
	node_id_t dnode_id, new_node_id;

	ino_to_node_id(dino, &dnode_id);
	ino_to_node_id(ino, &new_node_id);

	RC_WRAP_LABEL(rc, aborted, kvtree_attach, efs_fs->kvtree, &dnode_id,
		      &new_node_id, &k_name);

	RC_WRAP_LABEL(rc, aborted, efs_kvnode_load, &node, efs_fs->kvtree, ino);
	RC_WRAP_LABEL(rc, aborted, efs_update_stat, &node,
		      STAT_CTIME_SET|STAT_INCR_LINK);

	log_trace("EXIT: rc=%d ino=%llu dino=%llu dname=%s", rc, *ino, *dino, dname);
	RC_WRAP(kvs_end_transaction, kvstor, &index);
	return rc;
aborted:
	kvnode_fini(&node);
	kvs_discard_transaction(kvstor, &index);
	log_trace("EXIT: rc=%d ino=%llu dino=%llu dname=%s", rc, *ino, *dino, dname);
	return rc;
}

static inline bool efs_file_has_links(struct stat *stat)
{
	return stat->st_nlink > 0;
}

int efs_destroy_orphaned_file(struct efs_fs *efs_fs,
			      const efs_ino_t *ino)
{

	int rc;
	struct stat *stat = NULL;
	dstore_oid_t oid;
	struct kvstore *kvstor = kvstore_get();
	struct dstore *dstore = dstore_get();
	struct kvs_idx index;
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(kvstor && dstore);

	index = efs_fs->kvtree->index;

	RC_WRAP_LABEL(rc, out, efs_kvnode_load, &node, efs_fs->kvtree, ino);
	RC_WRAP_LABEL(rc, out, efs_get_stat, &node, &stat);

	if (efs_file_has_links(stat)) {
		rc = 0;
		goto out;
	}

	kvs_begin_transaction(kvstor, &index);

	RC_WRAP_LABEL(rc, out, efs_del_stat, &node);

	if (S_ISLNK(stat->st_mode)) {
		/* Delete symlink */
		RC_WRAP_LABEL(rc, out, efs_del_sysattr, &node,
			      EFS_SYS_ATTR_SYMLINK);
	} else if (S_ISREG(stat->st_mode)) {
		RC_WRAP_LABEL(rc, out, efs_ino_to_oid, efs_fs, ino, &oid);
		RC_WRAP_LABEL(rc, out, dstore_obj_delete,
			      dstore, efs_fs, &oid);
		RC_WRAP_LABEL(rc, out, efs_del_oid, efs_fs, ino);
	} else {
		/* Impossible: rmdir handles DIR; LNK and REG are handled by
		 * this function, the other types cannot be created
		 * at all.
		 */
		dassert(0);
		log_err("Attempt to remove unsupported object type (%d)",
			(int) stat->st_mode);
	}
	/* TODO: Delete File Xattrs here */
	kvs_end_transaction(kvstor, &index);

out:
	kvnode_fini(&node);
	if (stat) {
		kvs_free(kvstor, stat);
	}

	if (rc != 0) {
		kvs_discard_transaction(kvstor, &index);
	}

	return rc;
}

int efs_rename(struct efs_fs *efs_fs, efs_cred_t *cred,
	       efs_ino_t *sino_dir, char *sname, const efs_ino_t *psrc,
	       efs_ino_t *dino_dir, char *dname, const efs_ino_t *pdst,
	       const struct efs_rename_flags *pflags)
{
	int rc;
	bool overwrite_dst = false;
	bool rename_inplace = false;
	bool is_dst_non_empty_dir = false;
	struct stat *stat = NULL;
	efs_ino_t sino;
	efs_ino_t dino;
	str256_t k_sname;
	str256_t k_dname;
	mode_t s_mode = 0;
	mode_t d_mode = 0;
	struct kvstore *kvstor = kvstore_get();
	struct kvnode snode = KVNODE_INIT_EMTPY,
		      dnode = KVNODE_INIT_EMTPY;
	const struct efs_rename_flags flags = pflags ? *pflags :
		(const struct efs_rename_flags) EFS_RENAME_FLAGS_INIT;
	node_id_t dnode_id;

	dassert(kvstor);
	dassert(cred);
	dassert(sino_dir && dino_dir);
	dassert(sname && dname);
	dassert(strlen(sname) <= NAME_MAX);
	dassert(strlen(dname) <= NAME_MAX);
	dassert((*sino_dir != *dino_dir || strcmp(sname, dname) != 0));
	dassert(efs_fs);

	str256_from_cstr(k_sname, sname, strlen(sname));
	str256_from_cstr(k_dname, dname, strlen(dname));

	rename_inplace = (*sino_dir == *dino_dir);

	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, cred, sino_dir,
		      EFS_ACCESS_DELETE_ENTITY);

	if (!rename_inplace) {
		RC_WRAP_LABEL(rc, out, efs_access, efs_fs, cred, dino_dir,
			      EFS_ACCESS_CREATE_ENTITY);
	}

	if (psrc) {
		sino = *psrc;
	} else {
		RC_WRAP_LABEL(rc, out, efs_lookup, efs_fs, cred, sino_dir, sname,
			      &sino);
	}

	if (pdst) {
		dino = *pdst;
		overwrite_dst = true;
	} else {
		rc = efs_lookup(efs_fs, cred, dino_dir, dname, &dino);
		if (rc < 0 && rc != -ENOENT) {
			goto out;
		}
		overwrite_dst = (rc != -ENOENT);
	}

	if (overwrite_dst) {
		/* Fetch 'st_mode' for source and destination. */
		RC_WRAP_LABEL(rc, out, efs_kvnode_load, &snode, efs_fs->kvtree,
			      &sino);
		RC_WRAP_LABEL(rc, out, efs_get_stat, &snode, &stat);
		s_mode = stat->st_mode;
		kvnode_fini(&snode);
		kvs_free(kvstor, stat);
		stat = NULL;
		RC_WRAP_LABEL(rc, out, efs_kvnode_load, &dnode, efs_fs->kvtree,
			      &dino);
		RC_WRAP_LABEL(rc, out, efs_get_stat, &dnode, &stat);
		d_mode = stat->st_mode;
		kvnode_fini(&dnode);
		kvs_free(kvstor, stat);
		if (S_ISDIR(s_mode) != S_ISDIR(d_mode)) {
			log_warn("Incompatible source and destination %d,%d.",
				 (int) s_mode, (int) d_mode);
			rc = -ENOTDIR;
			goto out;
		}
		RC_WRAP_LABEL(rc, out, ino_to_node_id, &dino, &dnode_id);
		if (S_ISDIR(d_mode)) {
			RC_WRAP_LABEL(rc, out, kvtree_has_children,
				      efs_fs->kvtree, &dnode_id,
				      &is_dst_non_empty_dir);
		}
		if (is_dst_non_empty_dir) {
			log_warn("Destination is not empty (%llu:%s)",
				 dino, dname);
			rc = -EEXIST;
			goto out;
		}

		if (S_ISDIR(d_mode)) {
			/* FIXME: rmdir() does not have an option to destroy
			 * a dir object (unlinked from the tree), therefore
			 * we might lose some data here if the following
			 * operations (relinking) fail.
			 */
			RC_WRAP_LABEL(rc, out, efs_rmdir, efs_fs, cred,
				      dino_dir, dname);
		} else {
			/* Make an ophaned file: it will be destoyed either
			 * at the end of the function or when the file
			 * will be closed.
			 */
			log_trace("Detaching a file from the tree "
				  "(%llu, %llu, %s)", *dino_dir, dino, dname);
			RC_WRAP_LABEL(rc, out, efs_detach, efs_fs, cred,
				      dino_dir, &dino, dname);
		}
	}

	if (rename_inplace) {
		/* a shortcut for renaming only a dentry
		 * without re-linking of the inodes.
		 */
		RC_WRAP_LABEL(rc, out, efs_tree_rename_link, efs_fs,
			      sino_dir, &sino, &k_sname, &k_dname);
	} else {
		RC_WRAP_LABEL(rc, out, efs_kvnode_load, &snode, efs_fs->kvtree,
			      &sino);
                RC_WRAP_LABEL(rc, out, efs_get_stat, &snode, &stat);
                s_mode = stat->st_mode;
                kvs_free(kvstor, stat);

		node_id_t snode_id, new_node_id;

		ino_to_node_id(sino_dir, &snode_id);
		ino_to_node_id(&sino, &new_node_id);

		RC_WRAP_LABEL(rc, out, kvtree_detach, efs_fs->kvtree, &snode_id,
			      &k_sname);

		RC_WRAP_LABEL(rc, out, kvtree_attach, efs_fs->kvtree, &dnode_id,
			      &new_node_id, &k_dname);

		if(S_ISDIR(s_mode)){
			RC_WRAP_LABEL(rc, out, efs_update_stat, &snode,
				      STAT_DECR_LINK);
                        RC_WRAP_LABEL(rc, out, efs_kvnode_load, &dnode,
				      efs_fs->kvtree, dino_dir);
                        RC_WRAP_LABEL(rc, out, efs_update_stat, &dnode,
				      STAT_INCR_LINK);
		}
	}

	if (overwrite_dst && !S_ISDIR(d_mode) && !flags.is_dst_open) {
		/* Remove the actual 'destination' object only if all
		 * previous operations have completed successfully.
		 */
		log_trace("Removing detached file (%llu)", dino);
		RC_WRAP_LABEL(rc, out, efs_destroy_orphaned_file, efs_fs, &dino);
	}

out:
	kvnode_fini(&snode);
	kvnode_fini(&dnode);
	return rc;
}

int efs_rmdir(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *parent, char *name)
{
	int rc;
	efs_ino_t ino = 0LL;
	bool is_non_empty_dir;
	str256_t kname;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;
	struct kvnode child_node = KVNODE_INIT_EMTPY,
		      parent_node = KVNODE_INIT_EMTPY;
	dstore_oid_t oid;
	node_id_t id;

	dassert(efs_fs && cred && parent && name && kvstor);
	dassert(strlen(name) <= NAME_MAX);

	index = efs_fs->kvtree->index;

	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, cred, parent,
		      EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_lookup, efs_fs, cred, parent, name,
		      &ino);

	RC_WRAP_LABEL(rc, out, ino_to_node_id, &ino, &id);

	/* Check if directory empty */
	RC_WRAP_LABEL(rc, out, kvtree_has_children, efs_fs->kvtree, &id,
		      &is_non_empty_dir);
	if (is_non_empty_dir) {
		 rc = -ENOTEMPTY;
		 log_debug("ctx=%p ino=%llu name=%s not empty", efs_fs,
			    ino, name);
		 goto out;
	}

	str256_from_cstr(kname, name, strlen(name));

	RC_WRAP_LABEL(rc, out, kvs_begin_transaction, kvstor, &index);
	/* Detach the inode */
	node_id_t pnode_id;

	ino_to_node_id(parent, &pnode_id);
	RC_WRAP_LABEL(rc, aborted, kvtree_detach, efs_fs->kvtree, &pnode_id,
		      &kname);

	/* Remove its stat */
	RC_WRAP_LABEL(rc, aborted, efs_kvnode_load, &child_node, efs_fs->kvtree,
		      &ino);
	RC_WRAP_LABEL(rc, aborted, efs_del_stat, &child_node);

	/* Child dir has a "hardlink" to the parent ("..") */
	RC_WRAP_LABEL(rc, aborted, efs_kvnode_load, &parent_node,
		      efs_fs->kvtree, parent);
	RC_WRAP_LABEL(rc, aborted, efs_update_stat, &parent_node,
		      STAT_DECR_LINK);

	RC_WRAP_LABEL(rc, aborted, efs_ino_to_oid, efs_fs, &ino, &oid);

	RC_WRAP_LABEL(rc, aborted, efs_del_oid, efs_fs, &ino);

	/* TODO: Remove all xattrs when kvsns_remove_all_xattr is implemented */
	RC_WRAP_LABEL(rc, out, kvs_end_transaction, kvstor, &index);

aborted:
	kvnode_fini(&child_node);
	kvnode_fini(&parent_node);

	if (rc < 0) {
		/* FIXME: error code is overwritten */
		RC_WRAP_LABEL(rc, out, kvs_discard_transaction, kvstor, &index);
	}

out:
	log_debug("EXIT efs_fs=%p ino=%llu name=%s rc=%d", efs_fs,
		   ino, name, rc);
	return rc;
}

int efs_unlink(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *dir,
	       efs_ino_t *fino, char *name)
{
	int rc;
	efs_ino_t ino;

	if (likely(fino != NULL)) {
		ino = *fino;
	} else {
		RC_WRAP_LABEL(rc, out, efs_lookup, efs_fs, cred, dir, name, &ino);
	}

	RC_WRAP_LABEL(rc, out, efs_detach, efs_fs, cred, dir, &ino, name);
	RC_WRAP_LABEL(rc, out, efs_destroy_orphaned_file, efs_fs, &ino);

out:
	return rc;
}

int efs_detach(struct efs_fs *efs_fs, const efs_cred_t *cred,
	       const efs_ino_t *parent, const efs_ino_t *obj,
	       const char *name)
{
	int rc;
	str256_t k_name;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(kvstor != NULL);

	index = efs_fs->kvtree->index;

	str256_from_cstr(k_name, name, strlen(name));
	RC_WRAP_LABEL(rc, out, efs_access, efs_fs, (efs_cred_t *) cred,
		      (efs_ino_t *) parent, EFS_ACCESS_DELETE_ENTITY);

	kvs_begin_transaction(kvstor, &index);

	node_id_t pnode_id;

	ino_to_node_id(parent, &pnode_id);
	RC_WRAP_LABEL(rc, out, kvtree_detach, efs_fs->kvtree, &pnode_id,
		      &k_name);

	RC_WRAP_LABEL(rc, out, efs_kvnode_load, &node, efs_fs->kvtree, obj);
	RC_WRAP_LABEL(rc, out, efs_update_stat, &node,
		      STAT_CTIME_SET|STAT_DECR_LINK);
	kvs_end_transaction(kvstor, &index);

out:
	kvnode_fini(&node);
	if (rc != 0) {
		kvs_discard_transaction(kvstor, &index);
	}
	return rc;
}
