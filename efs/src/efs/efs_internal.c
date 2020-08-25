/*
 * Filename:         efs_internal.c
 * Description:      EFS file system internal APIs
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h> /* PATH_MAX */
#include <sys/time.h>
#include <ini_config.h>
#include <common/log.h>
#include <common/helpers.h>
#include "efs.h"
#include "efs_internal.h"
#include <dstore.h>
#include <debug.h>
#include <common.h> /* likely */
#include <inttypes.h> /* PRIx64 */
#include "kvtree.h"
#include "kvnode.h"

static int efs_set_ino_no_gen(struct efs_fs *efs_fs, efs_ino_t ino);
static int efs_get_ino_no_gen(struct efs_fs *efs_fs, efs_ino_t *ino);
static int efs_del_ino_no_gen(struct efs_fs *efs_fs);

/** Get pointer to a const C-string owned by cortxfs_name string. */
static inline const char *efs_name_as_cstr(const str256_t *kname)
{
        return kname->s_str;
}

static inline const char *efs_key_type_to_str(efs_key_type_t ktype)
{
	switch (ktype) {
	case EFS_KEY_TYPE_DIRENT:
		return "dentry";
	case EFS_KEY_TYPE_PARENT:
		return "parentdir";
	case EFS_KEY_TYPE_STAT:
		return "stat";
	case EFS_KEY_TYPE_SYMLINK:
		return "link";
	case EFS_KEY_TYPE_INODE_KFID:
		return "oid";
	case EFS_KEY_TYPE_GI_INDEX: /* @todo remove when default fs is gone */
		return "index";
	case EFS_KEY_TYPE_FS_ID_FID:
		return "fid";
	case EFS_KEY_TYPE_FS_NAME:
		return "fsid";
	case EFS_KEY_TYPE_FS_ID:
		return "fsname";
	case EFS_KEY_TYPE_FS_ID_NEXT:
		return "fsidnext";
	case EFS_KEY_TYPE_INO_NUM_GEN:
		return "ino_counter";
	case EFS_KEY_TYPE_INVALID:
		return "<invalid>";
	}

	dassert(0);
	return "<fail>";
}

#define INODE_ATTR_KEY_INIT(__kfid, __ktype)		\
{							\
		.fid = __kfid,				\
		.md = {					\
			.type = __ktype,		\
			.version = EFS_VERSION_0,	\
		},					\
}

/* @todo rename this to INODE_ATTR_KEY_INIT once all the instances of
 * INODE_ATTR_KEY_INIT are replaced with INODE_ATTR_KEY_PTR_INIT */
#define INODE_ATTR_KEY_PTR_INIT(_key, _ino, _ktype)	\
{							\
		_key->fid.f_hi = (*_ino),		\
		_key->fid.f_lo = 0,			\
		_key->md.type = _ktype,			\
		_key->md.version = EFS_VERSION_0;	\
}

/* Access routines */
int efs_access_check(const efs_cred_t *cred, const struct stat *stat,
		     int flags)
{
	int check = 0;

	if (!stat || !cred)
		return -EINVAL;

	/* Root's superpowers */
	if (cred->uid == EFS_ROOT_UID)
		return 0;

	if (cred->uid == stat->st_uid) {
		/* skip access check of owner for set attribute.*/
		if(flags & EFS_ACCESS_SETATTR)
			return 0;
		if (flags & EFS_ACCESS_READ)
			check |= STAT_OWNER_READ;

		if (flags & EFS_ACCESS_WRITE)
			check |= STAT_OWNER_WRITE;

		if (flags & EFS_ACCESS_EXEC)
			check |= STAT_OWNER_EXEC;
	} else if (cred->gid == stat->st_gid) {
		if (flags & EFS_ACCESS_READ)
			check |= STAT_GROUP_READ;

		if (flags & EFS_ACCESS_WRITE)
			check |= STAT_GROUP_WRITE;

		if (flags & EFS_ACCESS_EXEC)
			check |= STAT_GROUP_EXEC;
	} else {
		if (flags & EFS_ACCESS_READ)
			check |= STAT_OTHER_READ;

		if (flags & EFS_ACCESS_WRITE)
			check |= STAT_OTHER_WRITE;

		if (flags & EFS_ACCESS_EXEC)
			check |= STAT_OTHER_EXEC;
	}

	if ((check & stat->st_mode) != check)
		return -EPERM;
	else
		return 0;

	/* Should not be reached */
	return -EPERM;
}

static int efs_set_ino_no_gen(struct efs_fs *efs_fs, efs_ino_t ino)
{
	int rc;
	buff_t value;

	dassert(efs_fs != NULL);

	buff_init(&value, &ino, sizeof (ino));

	RC_WRAP_LABEL(rc, out, efs_set_sysattr, efs_fs->root_node, value,
		      EFS_SYS_ATTR_INO_NUM_GEN);

out:
	log_trace("efs_set_ino_no_gen() ends, ino:%llu, rc:%d", ino, rc);
	return rc;
}

static int efs_get_ino_no_gen(struct efs_fs *efs_fs, efs_ino_t *ino)
{
	int rc;
	buff_t value;

	dassert(efs_fs != NULL);
	dassert(ino != NULL);

	buff_init(&value, NULL, 0);

	RC_WRAP_LABEL(rc, out, efs_get_sysattr, efs_fs->root_node, &value,
		      EFS_SYS_ATTR_INO_NUM_GEN);

	if (sizeof (*ino) != value.len) {
	    log_trace("invalid value size %zu, expected %zu",
		      value.len, sizeof (*ino));
	    goto out;
	}
	memcpy(ino, value.buf, sizeof (*ino));

out:
	log_trace("efs_get_ino_no_gen() ends, ino:%llu, rc:%d", *ino, rc);
	if (value.buf) {
		free(value.buf);
	}
	return rc;
}

static int efs_del_ino_no_gen(struct efs_fs *efs_fs)
{
	int rc;

	dassert(efs_fs != NULL);

	RC_WRAP_LABEL(rc, out, efs_del_sysattr, efs_fs->root_node,
		      EFS_SYS_ATTR_INO_NUM_GEN);

out:
	log_trace("efs_del_ino_no_gen() ends, rc:%d", rc);
	return rc;
}

int efs_kvnode_init(struct kvnode *node, struct kvtree *tree,
		    const efs_ino_t *ino, const struct stat *bufstat)
{
	int rc;
	node_id_t node_id;
	uint16_t size = sizeof(struct stat);

	dassert(tree);
	dassert(bufstat);
	/**
	 * A kvnode is identified by a 128 bit node id. However, currently efs
	 * uses 64 bit inodes in it's apis to identify the entities. The
	 * following api is a temporary arrangement to convert 64 bit inodes
	 * to 128 bit node ids.
	 * @TODO: Cleanup these when efs filehandle (or equivalent) is
	 * implemented. The filehandles would eventually use 128bit unique fids.
	 */
	ino_to_node_id(ino, &node_id);

	rc = kvnode_init(tree, &node_id, (void *)bufstat, size, node);

	log_trace("efs_kvnode_init: " NODE_ID_F "uid: %d, gid: %d, mode: %04o,"
		  " rc : %d",
		  NODE_ID_P(&node->node_id),
		  bufstat->st_uid,
		  bufstat->st_gid,
		  bufstat->st_mode & 07777,
		  rc);

	return rc;
}

int efs_kvnode_load(struct kvnode *node, struct kvtree *tree,
		    const efs_ino_t *ino)
{
	int rc;
	node_id_t node_id;

	dassert(tree);
	/**
	 * A kvnode is identified by a 128 bit node id. However, currently efs
	 * uses 64 bit inodes in it's apis to identify the entities. The
	 * following api is a temporary arrangement to convert 64 bit inodes
	 * to 128 bit node ids.
	 * @TODO: Cleanup these when efs filehandle (or equivalent) is
	 * implemented. The filehandles would eventually use 128bit unique fids.
	 */
	ino_to_node_id(ino, &node_id);

	rc = kvnode_load(tree, &node_id, node);

	log_trace("efs_load_kvnode: " NODE_ID_F " rc : %d",
		  NODE_ID_P(&node->node_id), rc);

	return rc;
}

int efs_get_stat(const struct kvnode *node, struct stat **bufstat)
{
	int rc = 0;
	uint16_t attr_size;
	struct stat *attr_buff = NULL;
	struct kvstore *kvstore = kvstore_get();

	dassert(node);
	dassert(node->tree);
	dassert(node->basic_attr);

	attr_size = kvnode_get_basic_attr_buff(node, (void **)&attr_buff);

	dassert(attr_buff);
	dassert(attr_size == sizeof(struct stat));

	/* TODO: Can we avoid kvs_alloc and have separate API for efs? */
	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstore, (void **)bufstat,
		      attr_size);

	/* TODO: This extra memcpy is here because the lifetime of kvnode is
	 * limited by the scope of this function. When kvnode will be promoted
	 * upper to the argument list then we can return a weak reference to its
	 * internals (instead of loading kvsnode from kvstore). But right now we
	 * cannot do that, so that we just allocate a new buffer, copy data
	 * there and return this buffer to the caller
	*/
	memcpy(*bufstat, attr_buff, attr_size);

out:
	log_trace("efs_get_stat: " NODE_ID_F " rc : %d",
		  NODE_ID_P(&node->node_id), rc);

	return rc;
}

int efs_set_stat(struct kvnode *node)
{
	int rc = 0;

	dassert(node);
	dassert(node->tree);
	dassert(node->basic_attr);

	rc = kvnode_dump(node);

	log_trace("efs_set_stat" NODE_ID_F "rc : %d",
		  NODE_ID_P(&node->node_id), rc);

	return rc;
}

int efs_del_stat(struct kvnode *node)
{
	int rc;

	dassert(node);
	dassert(node->tree);
	dassert(node->basic_attr);

	rc = kvnode_delete(node);

	log_trace("efs_del_stat: " NODE_ID_F " rc : %d",
		  NODE_ID_P(&node->node_id), rc);

	return rc;
}

int efs_amend_stat(struct stat *stat, int flags)
{
	struct timeval t;
	int rc = 0;

	dassert(stat);

	rc = gettimeofday(&t, NULL);
	dassert(rc == 0);

	if (flags & STAT_ATIME_SET) {
		stat->st_atim.tv_sec = t.tv_sec;
		stat->st_atim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_MTIME_SET) {
		stat->st_mtim.tv_sec = t.tv_sec;
		stat->st_mtim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_CTIME_SET) {
		stat->st_ctim.tv_sec = t.tv_sec;
		stat->st_ctim.tv_nsec = 1000 * t.tv_usec;
	}

	if (flags & STAT_INCR_LINK) {
		/* nlink_t has to be an unsigned type, so overflow checking is
		 * simple.
		 */
		if (unlikely(stat->st_nlink + 1 == 0)) {
			rc = RC_WRAP_SET(-EINVAL);
			goto out;
		}
		if (unlikely(stat->st_nlink == EFS_MAX_LINK)) {
			rc = RC_WRAP_SET(-EINVAL);
			goto out;
		}
		stat->st_nlink += 1;
	}

	if (flags & STAT_DECR_LINK) {
		if (unlikely(stat->st_nlink == 0)) {
			rc = RC_WRAP_SET(-EINVAL);
			goto out;
		}

		stat->st_nlink -= 1;
	}

out:
	return rc;
}

int efs_update_stat(struct kvnode *node, int flags)
{
	int rc;
	uint16_t stat_size;
	struct stat *stat = NULL;

	stat_size = kvnode_get_basic_attr_buff(node, (void **)&stat);

	dassert(stat);
	dassert(stat_size == sizeof(struct stat));

	RC_WRAP_LABEL(rc, out, efs_amend_stat, stat, flags);
	RC_WRAP_LABEL(rc, out, efs_set_stat, node);

out:
	log_trace("efs_update_stat (%d) for " NODE_ID_F ", rc=%d",
		  flags, NODE_ID_P(&node->node_id), rc);

	return rc;
}

int efs_set_sysattr(const struct kvnode *node, const buff_t value,
                    enum efs_sys_attr_type attr_type)
{
	int rc;

	rc = kvnode_set_sys_attr(node, attr_type, value);

	log_trace("efs_set_sysattr:" OBJ_ID_F ", rc = %d",
		  OBJ_ID_P(&node->node_id), rc);

	return rc;
}

int efs_get_sysattr(const struct kvnode *node, buff_t *value,
                    enum efs_sys_attr_type attr_type)
{
	int rc;

	rc = kvnode_get_sys_attr(node, attr_type, value);

	log_trace("efs_get_sysattr:" OBJ_ID_F ", rc = %d",
		  OBJ_ID_P(&node->node_id), rc);

	return rc;
}

int efs_del_sysattr(const struct kvnode *node,
                    enum efs_sys_attr_type attr_type)
{
	int rc;

	rc = kvnode_del_sys_attr(node, attr_type);

	log_trace("efs_del_sysattr:" OBJ_ID_F ", rc = %d",
		  OBJ_ID_P(&node->node_id), rc);

	return rc;
}

int efs_ino_num_gen_init(struct efs_fs *efs_fs)
{
	int rc;

	dassert(efs_fs != NULL);

	rc = efs_set_ino_no_gen(efs_fs, EFS_ROOT_INODE_NUM_GEN_START);

	log_trace("efs_ino_num_gen_init() ends, rc:%d", rc);
	return rc;
}

int efs_ino_num_gen_fini(struct efs_fs *efs_fs)
{
	int rc;

	dassert(efs_fs != NULL);

	rc = efs_del_ino_no_gen(efs_fs);

	log_trace("efs_ino_num_gen_fini() ends, rc:%d", rc);
	return rc;
}

int efs_tree_rename_link(struct efs_fs *efs_fs,
			 const efs_ino_t *parent_ino,
			 const efs_ino_t *ino,
			 const str256_t *old_name,
			 const str256_t *new_name)
{
	int rc;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;
	struct kvnode parent_node = KVNODE_INIT_EMTPY;
	node_id_t id, pid;

	dassert(kvstor != NULL);

	index = efs_fs->kvtree->index;

	RC_WRAP_LABEL(rc, out, ino_to_node_id, parent_ino, &pid);
	// The caller must ensure that the entry exists prior renaming */
	dassert(kvtree_lookup(efs_fs->kvtree, &pid, old_name, NULL) == 0);

	RC_WRAP_LABEL(rc, out, ino_to_node_id, ino, &id);

	RC_WRAP_LABEL(rc, out, kvtree_detach, efs_fs->kvtree, &pid,
		      old_name);

	RC_WRAP_LABEL(rc, out, kvtree_attach, efs_fs->kvtree, &pid,
		      &id, new_name);

	// Update ctime stat
	RC_WRAP_LABEL(rc, cleanup, efs_kvnode_load, &parent_node,
		      efs_fs->kvtree, parent_ino);
	RC_WRAP_LABEL(rc, cleanup, efs_update_stat, &parent_node,
		      STAT_CTIME_SET);

cleanup:
	kvnode_fini(&parent_node);

out:
	log_debug("tree_rename(%p,pino=%llu,ino=%llu,o=%.*s,n=%.*s) = %d",
		  efs_fs, *parent_ino, *ino,
		  old_name->s_len, old_name->s_str,
		  new_name->s_len, new_name->s_str, rc);
	return rc;
}

static int efs_create_check_name(const char *name, size_t len)
{
	const char *parent = "..";

	if (len > NAME_MAX) {
		log_debug("Name too long %s", name);
		return  -E2BIG;
	}

	if (len == 1 && (name[0] == '.' || name[0] == '/')) {
		log_debug("File already exists: %s", name);
		return -EEXIST;
	}

	if (len == 2 && (strncmp(name, parent, 2) == 0)) {
		log_debug("File already exists: %s", name);
		return -EEXIST;
	}

	return 0;
}

int efs_next_inode(struct efs_fs *efs_fs, efs_ino_t *ino_out)
{
	int rc;
	efs_ino_t ino;

	dassert(efs_fs != NULL);
	dassert(ino_out != NULL);

	RC_WRAP_LABEL(rc, out, efs_get_ino_no_gen, efs_fs, &ino);
	ino++;
	RC_WRAP_LABEL(rc, out, efs_set_ino_no_gen, efs_fs, ino);

	*ino_out = ino;
out:
	log_trace("efs_next_inode() ends, ino = %llu, rc:%d", *ino_out, rc);
	return rc;
}

int efs_create_entry(struct efs_fs *efs_fs, efs_cred_t *cred, efs_ino_t *parent,
		     char *name, char *lnk, mode_t mode,
		     efs_ino_t *new_entry, enum efs_file_type type)
{
	int rc;
	struct stat bufstat;
	struct timeval t;
	size_t namelen;
	str256_t k_name;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;
	struct kvnode new_node = KVNODE_INIT_EMTPY,
	              parent_node = KVNODE_INIT_EMTPY;
	node_id_t new_node_id, parent_node_id;

	dassert(kvstor);
	index = efs_fs->kvtree->index;

	dassert(cred && parent && name && new_entry);

	namelen = strlen(name);
	if (namelen == 0)
		return -EINVAL;

	/* check if name is not '.' or '..' or '/' */
	rc = efs_create_check_name(name, namelen);
	if (rc != 0)
		return rc;

	if ((type == EFS_FT_SYMLINK) && (lnk == NULL))
		return -EINVAL;

	/* Return if file/dir/symlink already exists. */
	rc = efs_lookup(efs_fs, cred, parent, name, new_entry);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP(efs_next_inode, efs_fs, new_entry);
	RC_WRAP(kvs_begin_transaction, kvstor, &index);

	/* @todo: Alloc motr bufvecs and use it for key to avoid extra mem copy
	RC_WRAP_LABEL(rc, errfree, efs_alloc_dirent_key, namelen, &d_key); */

	str256_from_cstr(k_name, name, strlen(name));

	ino_to_node_id(new_entry, &new_node_id);
	ino_to_node_id(parent, &parent_node_id);

	RC_WRAP_LABEL(rc, errfree, kvtree_attach, efs_fs->kvtree,
	              &parent_node_id, &new_node_id, &k_name);

	/* Set the stats of the new file */
	memset(&bufstat, 0, sizeof(struct stat));
	bufstat.st_uid = cred->uid;
	bufstat.st_gid = cred->gid;
	bufstat.st_ino = *new_entry;

	if (gettimeofday(&t, NULL) != 0) {
		rc = -1;
		goto errfree;
	}

	bufstat.st_atim.tv_sec = t.tv_sec;
	bufstat.st_atim.tv_nsec = 1000 * t.tv_usec;

	bufstat.st_mtim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_mtim.tv_nsec = bufstat.st_atim.tv_nsec;

	bufstat.st_ctim.tv_sec = bufstat.st_atim.tv_sec;
	bufstat.st_ctim.tv_nsec = bufstat.st_atim.tv_nsec;

	switch (type) {
	case EFS_FT_DIR:
		bufstat.st_mode = S_IFDIR | mode;
		bufstat.st_nlink = 2;
		break;

	case EFS_FT_FILE:
		bufstat.st_mode = S_IFREG | mode;
		bufstat.st_nlink = 1;
		break;

	case EFS_FT_SYMLINK:
		bufstat.st_mode = S_IFLNK | mode;
		bufstat.st_nlink = 1;
		break;

	default:
		/* Should never happen */
		dassert(0);
		/* Handle error, since dasserts can be disabled in release */
		rc = -EINVAL;
		goto errfree;
	}
	/* Create node for new entry and set its stats */
	RC_WRAP_LABEL(rc, errfree, efs_kvnode_init, &new_node, efs_fs->kvtree,
	              new_entry, &bufstat);
	RC_WRAP_LABEL(rc, errfree, efs_set_stat, &new_node);

	if (type == EFS_FT_SYMLINK) {
		buff_t value;

		buff_init(&value, lnk, strnlen(lnk, PATH_MAX));

		/* Set symlink */
		RC_WRAP_LABEL(rc, errfree, efs_set_sysattr, &new_node, value,
		              EFS_SYS_ATTR_SYMLINK);
	}

	/* Load parent node to update its stats */
	RC_WRAP_LABEL(rc, errfree, efs_kvnode_load, &parent_node,
	              efs_fs->kvtree, parent);

	if (type == EFS_FT_DIR) {
		/* Child dir has a "hardlink" to the parent ("..") */
		RC_WRAP_LABEL(rc, errfree, efs_update_stat, &parent_node,
			      STAT_CTIME_SET | STAT_MTIME_SET | STAT_INCR_LINK);
	} else {
		RC_WRAP_LABEL(rc, errfree, efs_update_stat, &parent_node,
		              STAT_CTIME_SET | STAT_MTIME_SET);
	}

	RC_WRAP(kvs_end_transaction, kvstor, &index);

errfree:
	kvnode_fini(&new_node);
	kvnode_fini(&parent_node);

	if (rc != 0) {
		kvs_discard_transaction(kvstor, &index);
	}

	log_trace("efs_create_entry: rc=%d", rc);
	return rc;
}

#define INODE_KFID_KEY_INIT INODE_ATTR_KEY_PTR_INIT

int efs_set_ino_oid(struct efs_fs *efs_fs, efs_ino_t *ino, dstore_oid_t *oid)
{
	int rc;
	efs_inode_kfid_key_t *kfid_key = NULL;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(kvstor != NULL);

	index = efs_fs->kvtree->index;

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&kfid_key,
		      sizeof(*kfid_key));

	INODE_KFID_KEY_INIT(kfid_key, ino, EFS_KEY_TYPE_INODE_KFID);

	RC_WRAP_LABEL(rc, free_key, kvs_set, kvstor, &index, kfid_key,
		      sizeof(efs_inode_kfid_key_t), oid,
		      sizeof(efs_fid_t));

free_key:
	kvs_free(kvstor, kfid_key);

out:
	log_trace("efs_fs=%p ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d",
		   efs_fs, *ino, oid->f_hi, oid->f_lo, rc);
	return rc;
}

int efs_ino_to_oid(struct efs_fs *efs_fs, const efs_ino_t *ino, dstore_oid_t *oid)
{
	int rc;
	efs_inode_kfid_key_t  *kfid_key = NULL;
	uint64_t kfid_size = 0;
	dstore_oid_t *oid_val = NULL;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(kvstor != NULL);

	index = efs_fs->kvtree->index;

	dassert(ino != NULL);
	dassert(oid != NULL);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&kfid_key,
		      sizeof(*kfid_key));

	INODE_KFID_KEY_INIT(kfid_key, ino, EFS_KEY_TYPE_INODE_KFID);

	RC_WRAP_LABEL(rc, free_key, kvs_get, kvstor, &index, kfid_key,
		      sizeof(efs_inode_kfid_key_t), (void **)&oid_val,
		      &kfid_size);

	*oid = *oid_val;
	kvs_free(kvstor, oid_val);

free_key:
	kvs_free(kvstor, kfid_key);

out:
	log_trace("efs_fs=%p, *ino=%llu oid=%" PRIx64 ":%" PRIx64 " rc=%d, kfid_size=%" PRIu64 "",
		   efs_fs, *ino, oid->f_hi, oid->f_lo, rc, kfid_size);
	return rc;
}

int efs_del_oid(struct efs_fs *efs_fs, const efs_ino_t *ino)
{
	int rc;
	efs_inode_kfid_key_t *kfid_key = NULL;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(kvstor != NULL);

	index = efs_fs->kvtree->index;

	dassert(ino != NULL);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **)&kfid_key,
		      sizeof(*kfid_key));

	INODE_KFID_KEY_INIT(kfid_key, ino, EFS_KEY_TYPE_INODE_KFID);

	RC_WRAP_LABEL(rc, free_key, kvs_del, kvstor, &index, kfid_key,
		      sizeof(*kfid_key));

free_key:
	kvs_free(kvstor, kfid_key);

out:
	log_trace("efs_fs=%p, ino=%llu, rc=%d", efs_fs, *ino, rc);
	return rc;
}

