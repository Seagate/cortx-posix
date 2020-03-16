/*
 * Filename:         efs_ops.c
 * Description:      EOS file system operations

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system operations
*/

#include <common/log.h> /* log_debug() */
#include <kvstore.h> /* struct kvstore */
#include <efs.h> /* efs_get_stat() */
#include <debug.h> /* dassert() */
#include <common/helpers.h> /* RC_WRAP_LABEL() */
#include <string.h> /* memcpy() */
#include <sys/time.h> /* gettimeofday() */
#include <errno.h>  /* errno, -EINVAL */
#include <efs_fh.h> /* efs_fh */
#include "efs_internal.h" /* dstore_obj_delete() */
#include <common.h> /* likely */

int efs_getattr(efs_ctx_t ctx, const efs_cred_t *cred,
		const efs_ino_t *ino, struct stat *bufstat)
{
	int rc;
	struct stat *stat = NULL;
	struct kvstore *kvstor = kvstore_get();

	dassert(cred != NULL);
	dassert(ino != NULL);
	dassert(kvstor != NULL);

	RC_WRAP_LABEL(rc, out, efs_get_stat, ctx, ino, &stat);
	memcpy(bufstat, stat, sizeof(struct stat));
	kvs_free(kvstor, stat);
out:
	log_debug("ino=%d rc=%d", (int)bufstat->st_ino, rc);
	return rc;
}

int efs_setattr(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *ino,
		struct stat *setstat, int statflag)
{
	struct stat bufstat;
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

	RC_WRAP_LABEL(rc, out, efs_getattr, ctx, cred, ino, &bufstat);
	RC_WRAP_LABEL(rc, out, efs_access, ctx, cred, ino, EFS_ACCESS_SETATTR);
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
	RC_WRAP_LABEL(rc, out, efs_set_stat, ctx, ino, &bufstat);
out:
	log_debug("rc=%d", rc);
	return rc;
}

int efs_access(efs_ctx_t ctx, const efs_cred_t *cred,
	       const efs_ino_t *ino, int flags)
{
	int rc = 0;
	struct stat stat;

	dassert(cred && ino);

	RC_WRAP_LABEL(rc, out, efs_getattr, ctx, cred, ino, &stat);
	RC_WRAP_LABEL(rc, out, efs_access_check, cred, &stat, flags);
out:
	return rc;
}

int efs_readdir(efs_ctx_t fs_ctx,
		const efs_cred_t *cred,
		const efs_ino_t *dir_ino,
		efs_readdir_cb_t cb,
		void *cb_ctx)
{
	int rc;

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, (efs_cred_t *) cred,
			(efs_ino_t *) dir_ino,
			EFS_ACCESS_LIST_DIR);

	RC_WRAP_LABEL(rc, out, efs_tree_iter_children, fs_ctx, dir_ino,
			cb, cb_ctx);

	RC_WRAP_LABEL(rc, out, efs_update_stat, fs_ctx, dir_ino,
			STAT_ATIME_SET);

out:
	return rc;
}

int efs_mkdir(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *parent, char *name,
	      mode_t mode, efs_ino_t *newdir)
{
	RC_WRAP(efs_access, ctx, cred, parent, EFS_ACCESS_WRITE);

	return efs_create_entry(ctx, cred, parent, name, NULL,
			mode, newdir, EFS_FT_DIR);
}

int efs_lookup(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *parent,
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

	RC_WRAP_LABEL(rc, out, efs_fh_from_ino, ctx, parent, NULL, &parent_fh);
	RC_WRAP_LABEL(rc, out, efs_fh_lookup, cred, parent_fh, name, &fh);

	*ino = *efs_fh_ino(fh);

out:
	efs_fh_destroy(parent_fh);
	efs_fh_destroy(fh);
	return rc;
}

int efs_readlink(efs_ctx_t fs_ctx, efs_cred_t *cred, efs_ino_t *lnk,
		 char *content, size_t *size)
{
	int rc;
	void *lnk_content_buf = NULL;
	size_t content_size;
	struct kvstore *kvstor = kvstore_get();

	dassert(kvstor);

	log_trace("ENTER: symlink_ino=%llu", *lnk);
	dassert(cred && lnk && size);
	dassert(*size != 0);

	RC_WRAP_LABEL(rc, errfree, efs_get_symlink, fs_ctx, lnk,
		      &lnk_content_buf, &content_size);

	if (content_size > *size) {
		rc = -ENOBUFS;
		goto errfree;
	}

	memcpy(content, lnk_content_buf, content_size);
	*size = content_size;
	RC_WRAP_LABEL(rc, errfree, efs_update_stat, fs_ctx, lnk,
		      STAT_ATIME_SET);
	log_debug("Got link: content='%.*s'", (int) *size, content);
	rc = 0;

errfree:
	kvs_free(kvstor, lnk_content_buf);
	log_trace("EXIT: rc=%d", rc);
	return rc;
}
/** Default mode for a symlink object.
 * Here is a quote from `man 7 symlink`:
 *        On Linux, the permissions of a symbolic link are not used in any
 *        operations; the permissions are always 0777 (read, write, and execute
 *        for all user categories), and can't be changed.
 */
#define EFS_SYMLINK_MODE 0777

int efs_symlink(efs_ctx_t fs_ctx, efs_cred_t *cred, efs_ino_t *parent,
		char *name, char *content, efs_ino_t *newlnk)
{
	int rc;

	log_trace("ENTER: name=%s", name);
	dassert(cred && parent && name && content && newlnk);

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, parent, EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_create_entry, fs_ctx, cred, parent, name, content,
		      EFS_SYMLINK_MODE, newlnk, EFS_FT_SYMLINK);

	RC_WRAP_LABEL(rc, out, efs_update_stat, fs_ctx, parent, STAT_MTIME_SET|STAT_CTIME_SET);

out:
	log_trace("name=%s content=%s rc=%d", name, content, rc);
	return rc;
}


int efs_link(efs_ctx_t fs_ctx, efs_cred_t *cred, efs_ino_t *ino,
	     efs_ino_t *dino, char *dname)
{
	int rc;
	efs_ino_t tmpino = 0LL;
	str256_t k_name;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(cred && ino && dname && dino && kvstor);

	index.index_priv = fs_ctx;

	log_trace("ENTER: ino=%llu dino=%llu dname=%s", *ino, *dino, dname);
	RC_WRAP(kvs_begin_transaction, kvstor, &index);
	RC_WRAP_LABEL(rc, aborted, efs_access, fs_ctx, cred, dino, EFS_ACCESS_WRITE);

	rc = efs_lookup(fs_ctx, cred, dino, dname, &tmpino);
	if (rc == 0)
		return -EEXIST;

	str256_from_cstr(k_name, dname, strlen(dname));
	RC_WRAP_LABEL(rc, aborted, efs_tree_attach, fs_ctx, dino, ino, &k_name);

	RC_WRAP_LABEL(rc, aborted, efs_update_stat, fs_ctx, ino,
		      STAT_CTIME_SET|STAT_INCR_LINK);

	log_trace("EXIT: rc=%d ino=%llu dino=%llu dname=%s", rc, *ino, *dino, dname);
	RC_WRAP(kvs_end_transaction, kvstor, &index);
	return rc;
aborted:
	kvs_discard_transaction(kvstor, &index);
	log_trace("EXIT: rc=%d ino=%llu dino=%llu dname=%s", rc, *ino, *dino, dname);
	return rc;
}

static inline bool efs_file_has_links(struct stat *stat)
{
	return stat->st_nlink > 0;
}

int efs_destroy_orphaned_file(efs_fs_ctx_t fs_ctx,
			      const efs_ino_t *ino)
{

	int rc;
	struct stat *stat = NULL;
	dstore_oid_t oid;
	struct kvstore *kvstor = kvstore_get();
	struct dstore *dstore = dstore_get();
	struct kvs_idx index;

	dassert(kvstor && dstore);

	index.index_priv = fs_ctx;

	RC_WRAP_LABEL(rc, out, efs_get_stat, fs_ctx, ino, &stat);

	if (efs_file_has_links(stat)) {
		rc = 0;
		goto out;
	}

	kvs_begin_transaction(kvstor, &index);
	RC_WRAP_LABEL(rc, out, efs_del_stat, fs_ctx, ino);
	if (S_ISLNK(stat->st_mode)) {
		RC_WRAP_LABEL(rc, out, efs_del_symlink, fs_ctx, ino);
	} else if (S_ISREG(stat->st_mode)) {
		RC_WRAP_LABEL(rc, out, efs_ino_to_oid, fs_ctx, ino, &oid);
		RC_WRAP_LABEL(rc, out, dstore_obj_delete,
			      dstore, fs_ctx, &oid);
		RC_WRAP_LABEL(rc, out, efs_del_oid, fs_ctx, ino);
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
	if (stat) {
		kvs_free(kvstor, stat);
	}

	if (rc != 0) {
		kvs_discard_transaction(kvstor, &index);
	}

	return rc;
}

int efs_rename(efs_fs_ctx_t fs_ctx, efs_cred_t *cred,
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
	const struct efs_rename_flags flags = pflags ? *pflags :
		(const struct efs_rename_flags) EFS_RENAME_FLAGS_INIT;

	dassert(kvstor);
	dassert(cred);
	dassert(sino_dir && dino_dir);
	dassert(sname && dname);
	dassert(strlen(sname) <= NAME_MAX);
	dassert(strlen(dname) <= NAME_MAX);
	dassert((*sino_dir != *dino_dir || strcmp(sname, dname) != 0));
	dassert(fs_ctx);

	str256_from_cstr(k_sname, sname, strlen(sname));
	str256_from_cstr(k_dname, dname, strlen(dname));

	rename_inplace = (*sino_dir == *dino_dir);

	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, sino_dir,
		      EFS_ACCESS_DELETE_ENTITY);

	if (!rename_inplace) {
		RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, cred, dino_dir,
			      EFS_ACCESS_CREATE_ENTITY);
	}

	if (psrc) {
		sino = *psrc;
	} else {
		RC_WRAP_LABEL(rc, out, efs_lookup, fs_ctx, cred, sino_dir, sname,
			      &sino);
	}

	if (pdst) {
		dino = *pdst;
		overwrite_dst = true;
	} else {
		rc = efs_lookup(fs_ctx, cred, dino_dir, dname, &dino);
		if (rc < 0 && rc != -ENOENT) {
			goto out;
		}
		overwrite_dst = (rc != -ENOENT);
	}

	if (overwrite_dst) {
		/* Fetch 'st_mode' for source and destination. */
		RC_WRAP_LABEL(rc, out, efs_get_stat, fs_ctx, &sino, &stat);
		s_mode = stat->st_mode;
		kvs_free(kvstor, stat);
		stat = NULL;
		RC_WRAP_LABEL(rc, out, efs_get_stat, fs_ctx, &dino, &stat);
		d_mode = stat->st_mode;
		kvs_free(kvstor, stat);

		if (S_ISDIR(s_mode) != S_ISDIR(d_mode)) {
			log_warn("Incompatible source and destination %d,%d.",
				 (int) s_mode, (int) d_mode);
			rc = -ENOTDIR;
			goto out;
		}
		if (S_ISDIR(d_mode)) {
			RC_WRAP_LABEL(rc, out, efs_tree_has_children, fs_ctx,
				      &dino, &is_dst_non_empty_dir);
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
			RC_WRAP_LABEL(rc, out, efs_rmdir, fs_ctx, cred,
				      dino_dir, dname);
		} else {
			/* Make an ophaned file: it will be destoyed either
			 * at the end of the function or when the file
			 * will be closed.
			 */
			log_trace("Detaching a file from the tree "
				  "(%llu, %llu, %s)", *dino_dir, dino, dname);
			RC_WRAP_LABEL(rc, out, efs_detach, fs_ctx, cred,
				      dino_dir, &dino, dname);
		}
	}

	if (rename_inplace) {
		/* a shortcut for renaming only a dentry
		 * without re-linking of the inodes.
		 */
		RC_WRAP_LABEL(rc, out, efs_tree_rename_link, fs_ctx,
			      sino_dir, &sino, &k_sname, &k_dname);
	} else {
                RC_WRAP_LABEL(rc, out, efs_get_stat, fs_ctx, &sino, &stat);
                s_mode = stat->st_mode;
                kvs_free(kvstor, stat);

		RC_WRAP_LABEL(rc, out, efs_tree_detach, fs_ctx, sino_dir,
			      &sino, &k_sname);
		RC_WRAP_LABEL(rc, out, efs_tree_attach, fs_ctx, dino_dir,
			      &sino, &k_dname);
		if(S_ISDIR(s_mode)){
			RC_WRAP_LABEL(rc, out, efs_update_stat, fs_ctx, sino_dir,
				      STAT_DECR_LINK);
                        RC_WRAP_LABEL(rc, out, efs_update_stat, fs_ctx, dino_dir,
				      STAT_INCR_LINK);
		}
	}

	if (overwrite_dst && !S_ISDIR(d_mode) && !flags.is_dst_open) {
		/* Remove the actual 'destination' object only if all
		 * previous operations have completed successfully.
		 */
		log_trace("Removing detached file (%llu)", dino);
		RC_WRAP_LABEL(rc, out, efs_destroy_orphaned_file, fs_ctx, &dino);
	}

out:
	return rc;
}

int efs_rmdir(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *parent, char *name)
{
	int rc;
	efs_ino_t ino = 0LL;
	bool is_non_empty_dir;
	str256_t kname;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(ctx && cred && parent && name && kvstor);
	dassert(strlen(name) <= NAME_MAX);

	index.index_priv = ctx;

	RC_WRAP_LABEL(rc, out, efs_access, ctx, cred, parent,
		      EFS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, efs_lookup, ctx, cred, parent, name,
		      &ino);

	RC_WRAP_LABEL(rc, out, efs_tree_has_children, ctx,
		      &ino, &is_non_empty_dir);

	/* Check if directory empty */
	if (is_non_empty_dir) {
		 rc = -ENOTEMPTY;
		 log_debug("ctx=%p ino=%llu name=%s not empty", ctx,
			    ino, name);
		 goto out;
	}

	str256_from_cstr(kname, name, strlen(name));

	RC_WRAP_LABEL(rc, out, kvs_begin_transaction, kvstor, &index);
	/* Detach the inode */
	RC_WRAP_LABEL(rc, aborted, efs_tree_detach, ctx, parent,
		      &ino, &kname);

	/* Remove its stat */
	RC_WRAP_LABEL(rc, aborted, efs_del_stat, ctx, &ino);

	/* Child dir has a "hardlink" to the parent ("..") */
	RC_WRAP_LABEL(rc, aborted, efs_update_stat, ctx, parent,
		      STAT_DECR_LINK);

	/* TODO: Remove all xattrs when kvsns_remove_all_xattr is implemented */
	RC_WRAP_LABEL(rc, out, kvs_end_transaction, kvstor, &index);

aborted:
	if (rc < 0) {
		/* FIXME: error code is overwritten */
		RC_WRAP_LABEL(rc, out, kvs_discard_transaction, kvstor, &index);
	}

out:
	log_debug("EXIT ctx=%p ino=%llu name=%s rc=%d", ctx,
		   ino, name, rc);
	return rc;
}

int efs_unlink(efs_ctx_t ctx, efs_cred_t *cred, efs_ino_t *dir,
	       efs_ino_t *fino, char *name)
{
	int rc;
	efs_ino_t ino;

	if (likely(fino != NULL)) {
		ino = *fino;
	} else {
		RC_WRAP_LABEL(rc, out, efs_lookup, ctx, cred, dir, name, &ino);
	}

	RC_WRAP_LABEL(rc, out, efs_detach, ctx, cred, dir, &ino, name);
	RC_WRAP_LABEL(rc, out, efs_destroy_orphaned_file, ctx, &ino);

out:
	return rc;
}

int efs_detach(efs_ctx_t fs_ctx, const efs_cred_t *cred,
	       const efs_ino_t *parent, const efs_ino_t *obj,
	       const char *name)
{
	int rc;
	str256_t k_name;
	struct kvstore *kvstor = kvstore_get();
	struct kvs_idx index;

	dassert(kvstor != NULL);

	index.index_priv = fs_ctx;

	str256_from_cstr(k_name, name, strlen(name));
	RC_WRAP_LABEL(rc, out, efs_access, fs_ctx, (efs_cred_t *) cred,
		      (efs_ino_t *) parent, EFS_ACCESS_DELETE_ENTITY);

	kvs_begin_transaction(kvstor, &index);
	RC_WRAP_LABEL(rc, out, efs_tree_detach, fs_ctx, parent, obj, &k_name);
	RC_WRAP_LABEL(rc, out, efs_update_stat, fs_ctx, obj,
		      STAT_CTIME_SET|STAT_DECR_LINK);
	kvs_end_transaction(kvstor, &index);

out:
	if (rc != 0) {
		kvs_discard_transaction(kvstor, &index);
	}
	return rc;
}
