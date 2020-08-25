/*
 * Filename: cortxfs_fh.h
 * Description: CORTXFS File Handle API implementation.
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

#include <kvstore.h>
#include "cortxfs.h"
#include "cortxfs_internal.h"
#include "cortxfs_fh.h"
#include <debug.h>
#include <string.h> /* strcmp() */
#include <str.h> /* str256_t */
#include <errno.h> /* EINVAL */
#include <common/helpers.h> /* RC_WRAP_LABEL */
#include <common.h> /* unlikely */
#include <common/log.h> /* log_err() */
#include "kvtree.h" /* kvtree_lookup() */

/**
 * A unique key to be used in containers (maps, sets).
 * TODO: It will be replaced by fid of the file
 * or a combination of FSFid+FileFid.
 * NOTE: This is not to be stored in kvs(kvstore).
 * This is only in memory.
 * @todo: find a better name than cfs_fh_key,
 * since *_key names are used by keys stored in kvs.
 */
struct cfs_fh_key {
	struct cfs_fs *fs;
	uint64_t file;
};

/**
 * This is the cortxfs file handle.
 */
struct cfs_fh {
	struct cfs_fs *fs;

	/** Inode number of the FH.
	 * TODO: will be be eliminated and stat->st_ino used instead
	 */
	cfs_ino_t ino;

	/** Attributes of the file handle object.
	 * Pointer is used here as a reference type but not as
	 * an optional type, i.e. it must not be NULL.
	 */
	struct stat *stat;

	/* A unique key to be used in containers (maps, sets).
	 * TODO: It will be replaced by fid of the file
	 * or a combination of FSFid+FileFid.
	 */
	struct cfs_fh_key key;

	/* TDB: void *symlik_contents; */
	/* TODO: Symlink target and any other attributes might be added
	 * here to avoid unnecessary calls to KVS.
	 */
	/* TBD: cortxfs_fid_t fid; */
	/* TODO: FIDs are not used yet */
};

/** Initialize an empty invalid FH instance */
#define CFS_FH_INIT (struct cfs_fh) { .ino = 0 }

static inline
bool cfs_fh_invariant(const struct cfs_fh *fh)
{
	/* A FH should have
	 *	- Filesystem pointer set.
	 *	- Non-Zero Inode (FIXME: add 'ino != 1' as well?)
	 *	- stat buffer to be set.
	 */
	return fh->fs != NULL && fh->ino != 0 && fh->stat != NULL &&
		fh->stat->st_ino == fh->ino;
}

struct cfs_fh_serialized {
	uint64_t fsid;
	cfs_ino_t ino_num;
};

static inline
void cfs_fh_init_key(struct cfs_fh *fh)
{
	fh->key.file = fh->ino;
	fh->key.fs = fh->fs;
}

/******************************************************************************/
int cfs_fh_from_ino(struct cfs_fs *fs, const cfs_ino_t *ino_num,
		    const struct stat *stat, struct cfs_fh **fh)
{
	int rc;
	struct cfs_fh *newfh = NULL;
	struct kvstore *kvstor =  kvstore_get();
	struct kvnode node = KVNODE_INIT_EMTPY;

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **) &newfh,
		      sizeof(struct cfs_fh));
	*newfh = CFS_FH_INIT;
	if (stat == NULL) {
		RC_WRAP_LABEL(rc, out, cfs_kvnode_load, &node, fs->kvtree,
			      ino_num);
		RC_WRAP_LABEL(rc, out, cfs_get_stat, &node, &newfh->stat);
	} else {
		RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor,
			      (void **) &newfh->stat, sizeof(struct stat));
		memcpy(newfh->stat, stat, sizeof(struct stat));
	}
	newfh->ino = *ino_num;
	newfh->fs = fs;
	cfs_fh_init_key(newfh);
	*fh = newfh;
	newfh = NULL;

out:
	kvnode_fini(&node);
	if (unlikely(newfh)) {
		cfs_fh_destroy(newfh);
	}
	return rc;
}

int cfs_fh_lookup(const cfs_cred_t *cred, struct cfs_fh *fh, const char *name,
		    struct cfs_fh **pfh)
{
	int rc;
	str256_t kname;
	struct cfs_fh *newfh = NULL;
	struct kvstore *kvstor = kvstore_get();
	struct kvnode node = KVNODE_INIT_EMTPY;
	node_id_t pid, id;

	dassert(cred && fh && name && pfh && kvstor);

	RC_WRAP_LABEL(rc, out, cfs_access_check,
		      (cfs_cred_t *) cred, fh->stat, CFS_ACCESS_READ);

	if ((fh->ino == CFS_ROOT_INODE) && (strcmp(name, "..") == 0)) {
		*pfh = fh;
		goto out;
	}

	str256_from_cstr(kname, name, strlen(name));

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **) &newfh,
		      sizeof(struct cfs_fh));
	*newfh = CFS_FH_INIT;

	RC_WRAP_LABEL(rc, out, ino_to_node_id, &fh->ino, &pid);

	RC_WRAP_LABEL(rc, out, kvtree_lookup, fh->fs->kvtree, &pid, &kname,
		      &id);

	node_id_to_ino(&id, &newfh->ino);
	dassert(newfh->ino >= CFS_ROOT_INODE);

	newfh->fs = fh->fs;
	RC_WRAP_LABEL(rc, out, cfs_kvnode_load, &node, newfh->fs->kvtree,
		      &newfh->ino);
	RC_WRAP_LABEL(rc, out, cfs_get_stat, &node, &newfh->stat);
	cfs_fh_init_key(newfh);
	*pfh = newfh;
	newfh = NULL;

	/* FIXME: Shouldn't we update parent.atime here? */
out:
	kvnode_fini(&node);
	if (newfh) {
		cfs_fh_destroy(newfh);
	}
	return rc;
}

void cfs_fh_destroy(struct cfs_fh *fh)
{
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	/* support the free() semantic */
	if (fh) {
		kvs_free(kvstor, fh->stat);
		kvs_free(kvstor, fh);
	}
}

cfs_ino_t *cfs_fh_ino(struct cfs_fh *fh)
{
	return &fh->ino;
}

struct stat *cfs_fh_stat(struct cfs_fh *fh)
{
	return fh->stat;
}

int cfs_fh_getroot(struct cfs_fs *fs, const cfs_cred_t *cred, struct cfs_fh **pfh)
{
	int rc;
	struct cfs_fh *fh = NULL;
	cfs_ino_t root_ino = CFS_ROOT_INODE;

	RC_WRAP_LABEL(rc, out, cfs_fh_from_ino, fs, &root_ino, NULL, &fh);
	RC_WRAP_LABEL(rc, out, cfs_access_check,
		      (cfs_cred_t *) cred, fh->stat, CFS_ACCESS_READ);
	*pfh = fh;
	fh = NULL;

out:
	if (unlikely(fh)) {
		cfs_fh_destroy(fh);
	}
	return rc;
}

int cfs_fh_serialize(const struct cfs_fh *fh, void* buffer, size_t max_size)
{
	int rc = 0;
	struct cfs_fh_serialized data = { .ino_num = 0 };

	dassert(cfs_fh_invariant(fh));

	if (max_size < sizeof(struct cfs_fh_serialized)) {
		rc = -ENOBUFS;
		goto out;
	}

	data.ino_num = fh->ino;
	/* fsid is ignored */

	memcpy(buffer, &data, sizeof(data));
	rc = sizeof(data);

out:
	return rc;
}

int cfs_fh_deserialize(struct cfs_fs *fs,
		       const cfs_cred_t *cred,
		       const void* buffer, size_t buffer_size,
		       struct cfs_fh** pfh)
{
	int rc = 0;
	struct cfs_fh_serialized data = { .ino_num = 0 };

	/* FIXME: We need to check if this function is a subject
	 * to access checks.
	 */
	(void) cred;

	if (buffer_size != sizeof(struct cfs_fh_serialized)) {
		rc = -EINVAL;
		goto out;
	}

	memcpy(&data, buffer, sizeof(data));

	/* data.fsid is ignored */

	RC_WRAP_LABEL(rc, out, cfs_fh_from_ino, fs, &data.ino_num, NULL, pfh);

out:
	return rc;
}

size_t cfs_fh_serialized_size(void)
{
	return sizeof(struct cfs_fh_serialized);
}

int cfs_fh_ser_with_fsid(const struct cfs_fh *fh, uint64_t fsid, void *buffer,
			 size_t max_size)
{
	int rc = 0;
	struct cfs_fh_serialized data = { .ino_num = 0 };

	dassert(cfs_fh_invariant(fh));

	if (max_size < sizeof(struct cfs_fh_serialized)) {
		rc = -ENOBUFS;
		goto out;
	}

	data.ino_num = fh->ino;
	data.fsid = fsid;

	memcpy(buffer, &data, sizeof(data));
	rc = sizeof(data);

out:
	return rc;
}

void cfs_fh_key(const struct cfs_fh *fh, void **pbuffer, size_t *psize)
{
	*pbuffer = (void *) &fh->key;
	*psize = sizeof(fh->key);
}

