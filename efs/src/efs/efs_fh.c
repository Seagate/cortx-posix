/*
 * Filename: efs_fh.h
 * Description: EFS File Handle API implementation.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 */

#include <kvstore.h>
#include <efs.h>
#include <efs_fh.h>
#include <efs.h>
#include <efs_fh.h>
#include <debug.h>
#include <string.h> /* strcmp() */
#include <str.h> /* str256_t */
#include <errno.h> /* EINVAL */
#include <common/helpers.h> /* RC_WRAP_LABEL */
#include <common.h> /* unlikely */
#include <common/log.h> /* log_err() */


/**
 * A unique key to be used in containers (maps, sets).
 * TODO: It will be replaced by fid of the file
 * or a combination of FSFid+FileFid.
 * NOTE: This is not to be stored in kvs(kvstore).
 * This is only in memory.
 * @todo: find a better name than efs_fh_key,
 * since *_key names are used by keys stored in kvs.
 */
struct efs_fh_key {
	efs_ctx_t fs;
	uint64_t file;
};

/**
 * This is the efs file handle.
 */
struct efs_fh {
	efs_ctx_t fs;

	/** Inode number of the FH.
	 * TODO: will be be eliminated and stat->st_ino used instead
	 */
	efs_ino_t ino;

	/** Attributes of the file handle object.
	 * Pointer is used here as a reference type but not as
	 * an optional type, i.e. it must not be NULL.
	 */
	struct stat *stat;

	/* A unique key to be used in containers (maps, sets).
	 * TODO: It will be replaced by fid of the file
	 * or a combination of FSFid+FileFid.
	 */
	struct efs_fh_key key;

	/* TDB: void *symlik_contents; */
	/* TODO: Symlink target and any other attributes might be added
	 * here to avoid unnecessary calls to KVS.
	 */
	/* TBD: kvsns_fid_t fid; */
	/* TODO: FIDs are not used yet */
};

/** Initialize an empty invalid FH instance */
#define EFS_FH_INIT (struct efs_fh) { .ino = 0 }

static inline
bool efs_fh_invariant(const struct efs_fh *fh)
{
	/* A FH should have
	 *	- Filesystem pointer set.
	 *	- Non-Zero Inode (FIXME: add 'ino != 1' as well?)
	 *	- stat buffer to be set.
	 */
	return fh->fs != NULL && fh->ino != 0 && fh->stat != NULL &&
		fh->stat->st_ino == fh->ino;
}

struct efs_fh_serialized {
	uint64_t fsid;
	efs_ino_t ino_num;
};

static inline
void efs_fh_init_key(struct efs_fh *fh)
{
	fh->key.file = fh->ino;
	fh->key.fs = fh->fs;
}

/******************************************************************************/
int efs_fh_from_ino(efs_ctx_t fs, const efs_ino_t *ino_num,
		    const struct stat *stat, struct efs_fh **fh)
{
	int rc;
	struct efs_fh *newfh = NULL;
	struct kvstore *kvstor =  kvstore_get();

	dassert(kvstor);

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **) &newfh,
		      sizeof(struct efs_fh));
	*newfh = EFS_FH_INIT;
	if (stat == NULL) {
		RC_WRAP_LABEL(rc, out, efs_get_stat, fs, ino_num, &newfh->stat);
	} else {
		RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor,
			      (void **) &newfh->stat, sizeof(struct stat));
		memcpy(newfh->stat, stat, sizeof(struct stat));
	}
	newfh->ino = *ino_num;
	newfh->fs = fs;
	efs_fh_init_key(newfh);
	*fh = newfh;
	newfh = NULL;

out:
	if (unlikely(newfh)) {
		efs_fh_destroy(newfh);
	}
	return rc;
}

int efs_fh_lookup(const efs_cred_t *cred, struct efs_fh *fh, const char *name,
		    struct efs_fh **pfh)
{
	int rc;
	str256_t kname;
	struct efs_fh *newfh = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(cred && fh && name && pfh && kvstor);

	RC_WRAP_LABEL(rc, out, efs_access_check,
		      (efs_cred_t *) cred, fh->stat, EFS_ACCESS_READ);

	if ((fh->ino == EFS_ROOT_INODE) && (strcmp(name, "..") == 0)) {
		*pfh = fh;
		goto out;
	}

	str256_from_cstr(kname, name, strlen(name));

	RC_WRAP_LABEL(rc, out, kvs_alloc, kvstor, (void **) &newfh,
		      sizeof(struct efs_fh));
	*newfh = EFS_FH_INIT;
	RC_WRAP_LABEL(rc, out, efs_tree_lookup, fh->fs, &fh->ino, &kname,
		      &newfh->ino);
	newfh->fs = fh->fs;
	RC_WRAP_LABEL(rc, out, efs_get_stat, newfh->fs, &newfh->ino,
		      &newfh->stat);
	efs_fh_init_key(newfh);
	*pfh = newfh;
	newfh = NULL;

	/* FIXME: Shouldn't we update parent.atime here? */
out:
	if (newfh) {
		efs_fh_destroy(newfh);
	}
	return rc;
}

void efs_fh_destroy(struct efs_fh *fh)
{
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	/* support the free() semantic */
	if (fh) {
		kvs_free(kvstor, fh->stat);
		kvs_free(kvstor, fh);
	}
}

efs_ino_t *efs_fh_ino(struct efs_fh *fh)
{
	return &fh->ino;
}

struct stat *efs_fh_stat(struct efs_fh *fh)
{
	return fh->stat;
}

int efs_fh_getroot(efs_ctx_t fs, const efs_cred_t *cred, struct efs_fh **pfh)
{
	int rc;
	struct efs_fh *fh = NULL;
	efs_ino_t root_ino = EFS_ROOT_INODE;

	RC_WRAP_LABEL(rc, out, efs_fh_from_ino, fs, &root_ino, NULL, &fh);
	RC_WRAP_LABEL(rc, out, efs_access_check,
		      (efs_cred_t *) cred, fh->stat, EFS_ACCESS_READ);
	*pfh = fh;
	fh = NULL;

out:
	if (unlikely(fh)) {
		efs_fh_destroy(fh);
	}
	return rc;
}

int efs_fh_serialize(const struct efs_fh *fh, void* buffer, size_t max_size)
{
	int rc = 0;
	struct efs_fh_serialized data = { .ino_num = 0 };

	dassert(efs_fh_invariant(fh));

	if (max_size < sizeof(struct efs_fh_serialized)) {
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

int efs_fh_deserialize(efs_ctx_t fs,
		       const efs_cred_t *cred,
		       const void* buffer, size_t buffer_size,
		       struct efs_fh** pfh)
{
	int rc = 0;
	struct efs_fh_serialized data = { .ino_num = 0 };

	/* FIXME: We need to check if this function is a subject
	 * to access checks.
	 */
	(void) cred;

	if (buffer_size != sizeof(struct efs_fh_serialized)) {
		rc = -EINVAL;
		goto out;
	}

	memcpy(&data, buffer, sizeof(data));

	/* data.fsid is ignored */

	RC_WRAP_LABEL(rc, out, efs_fh_from_ino, fs, &data.ino_num, NULL, pfh);

out:
	return rc;
}

size_t efs_fh_serialized_size(void)
{
	return sizeof(struct efs_fh_serialized);
}

int efs_fh_ser_with_fsid(const struct efs_fh *fh, uint64_t fsid, void *buffer,
			 size_t max_size)
{
	int rc = 0;
	struct efs_fh_serialized data = { .ino_num = 0 };

	dassert(efs_fh_invariant(fh));

	if (max_size < sizeof(struct efs_fh_serialized)) {
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

void efs_fh_key(const struct efs_fh *fh, void **pbuffer, size_t *psize)
{
	*pbuffer = (void *) &fh->key;
	*psize = sizeof(fh->key);
}

