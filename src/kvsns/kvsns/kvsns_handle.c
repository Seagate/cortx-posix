 /*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_handle.c
 * KVSNS: functions to manage handles
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include <kvsns/extstore.h>
#include "kvsns_internal.h"
#include "kvsns/log.h"

/*
 * @todo : In future, this would be an array of filesystem contexts indexed by
 * fs id. This array would be created and populated during kvsns start/init.
 */
static kvsns_fs_ctx_t kvsns_fs_ctx = KVSNS_NULL_FS_CTX;

int kvsns_fsstat(kvsns_fsstat_t *stat)
{
	char k[KLEN];
	int rc;

	if (!stat)
		return -EINVAL;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "*.stat");
	rc = kvsal_get_list_size(k);
	if (rc < 0)
		return rc;

	stat->nb_inodes = rc;
	return 0;
}

int kvsns_get_root(kvsns_ino_t *ino)
{
	if (!ino)
		return -EINVAL;

	*ino = KVSNS_ROOT_INODE;
	return 0;
}

int kvsns_mkdir(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		 mode_t mode, kvsns_ino_t *newdir)
{
	RC_WRAP(kvsns2_access, ctx, cred, parent, KVSNS_ACCESS_WRITE);

	return kvsns2_create_entry(ctx, cred, parent, name, NULL,
				   mode, newdir, KVSNS_DIR);
}

int kvsns_symlink(kvsns_fs_ctx_t fs_ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		  char *name, char *content, kvsns_ino_t *newlnk)
{
	int rc;

	log_trace("ENTER: name=%s", name);
	KVSNS_DASSERT(cred && parent && name && content && newlnk);

	RC_WRAP_LABEL(rc, out, kvsns2_access, fs_ctx, cred, parent, KVSNS_ACCESS_WRITE);

	RC_WRAP_LABEL(rc, out, kvsns2_create_entry, fs_ctx, cred, parent, name, content,
		      0, newlnk, KVSNS_SYMLINK);

	RC_WRAP_LABEL(rc, out, kvsns2_update_stat, fs_ctx, parent, STAT_MTIME_SET|STAT_CTIME_SET);

out:
	log_trace("name=%s content=%s rc=%d", name, content, rc);
	return rc;
}

int kvsns_readlink(kvsns_fs_ctx_t fs_ctx, kvsns_cred_t *cred, kvsns_ino_t *lnk,
		   char *content, size_t *size)
{
	int rc;
	void *lnk_content_buf = NULL;
	size_t content_size;

	log_trace("ENTER: lnk=%llu", *lnk);
	KVSNS_DASSERT(cred && lnk);

	RC_WRAP_LABEL(rc, errfree, kvsns_get_link, fs_ctx, lnk, &lnk_content_buf, &content_size);

	// TODO: 4K memory was allocated for the content as caller does not know the 
	// size of the content. If we want to malloc exact size instead of 4K
	// the APIs needs to be changed. Please refer EOS-259 for the details.

	// This is the only memcpy which copies data to NFA Ganesha buffer
	memcpy(content, lnk_content_buf, content_size);
	RC_WRAP_LABEL(rc, errfree, kvsns2_update_stat, fs_ctx, lnk, STAT_ATIME_SET);

errfree:
	kvsal_free(lnk_content_buf);
	log_trace("EXIT: lnk=%llu content=%s rc=%d", *lnk, content, rc);
	return rc;
}

int kvsns_rmdir(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name)
{
	int rc;
	char k[KLEN];
	kvsns_ino_t ino = 0LL;
	struct stat parent_stat;

	if (!cred || !parent || !name)
		return -EINVAL;

	memset(&parent_stat, 0, sizeof(parent_stat));

	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_WRITE);

	RC_WRAP(kvsns_lookup, cred, parent, name, &ino);

	RC_WRAP(kvsns_get_stat, parent, &parent_stat);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.dentries.*", ino);
	rc = kvsal_get_list_size(k);
	if (rc > 0)
		return -ENOTEMPTY;

	RC_WRAP(kvsal_begin_transaction);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.dentries.%s",
		 *parent, name);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.parentdir", ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", ino);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &parent_stat,
		      STAT_CTIME_SET|STAT_MTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, parent, &parent_stat);

	RC_WRAP(kvsal_end_transaction);

	/* Remove all associated xattr */
	RC_WRAP(kvsns_remove_all_xattr, cred, &ino);

	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

int kvsns_readdir(kvsns_fs_ctx_t fs_ctx,
		  const kvsns_cred_t *cred,
		  const kvsns_ino_t *dir_ino,
		  kvsns_readdir_cb_t cb,
		  void *cb_ctx)
{
	int rc;

	RC_WRAP_LABEL(rc, out, kvsns2_access, fs_ctx, (kvsns_cred_t *) cred,
		      (kvsns_ino_t *) dir_ino,
		      KVSNS_ACCESS_LIST_DIR);

	RC_WRAP_LABEL(rc, out, kvsns_tree_iter_children, fs_ctx, dir_ino,
		      cb, cb_ctx);

	RC_WRAP_LABEL(rc, out, kvsns2_update_stat, fs_ctx, dir_ino,
		      STAT_ATIME_SET);

out:
	return rc;
}

int kvsns_lookup(kvsns_cred_t *cred, kvsns_ino_t *parent, char *name,
		kvsns_ino_t *ino)
{
	char k[KLEN];
	char v[VLEN];

	if (!cred || !parent || !name || !ino)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, parent, KVSNS_ACCESS_READ);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.dentries.%s",
		 *parent, name);
	log_debug("%llu.dentries.%s", *parent, name);
	RC_WRAP(kvsal_get_char, k, v);

	sscanf(v, "%llu", ino);

	return 0;
}

int kvsns2_lookup(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
		  char *name, kvsns_ino_t *ino)

{
	int rc;
	kvsns_name_t kname;

	KVSNS_DASSERT(cred && parent && name && ino);

	RC_WRAP_LABEL(rc, out, kvsns_name_from_cstr, name, &kname);
	RC_WRAP_LABEL(rc, out, kvsns2_access, ctx, cred, parent, KVSNS_ACCESS_READ);
	RC_WRAP_LABEL(rc, out, kvsns_tree_lookup, ctx,
		      (const kvsns_ino_t *) parent, &kname, ino);

out:
	return rc;
}


int kvsns_lookupp(kvsns_cred_t *cred, kvsns_ino_t *dir, kvsns_ino_t *parent)
{
	char k[KLEN];
	char v[VLEN];

	if (!cred || !dir || !parent)
		return -EINVAL;

	RC_WRAP(kvsns_access, cred, dir, KVSNS_ACCESS_READ);

	memset(k, 0, KLEN);
	memset(v, 0, VLEN);
	snprintf(k, KLEN, "%llu.parentdir",
		 *dir);

	RC_WRAP(kvsal_get_char, k, v);

	sscanf(v, "%llu|", parent);

	return 0;
}

int kvsns_getattr(kvsns_cred_t *cred, kvsns_ino_t *ino, struct stat *bufstat)
{
	char k[KLEN];

	if (!cred || !ino || !bufstat)
		return -EINVAL;

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", *ino);
	return kvsal_get_stat(k, bufstat);
}

int kvsns2_getattr(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		   struct stat *bufstat)
{
	int rc;
	struct stat *stat = NULL;

	KVSNS_DASSERT(cred != NULL);
	KVSNS_DASSERT(ino != NULL);

	RC_WRAP_LABEL(rc, out, kvsns2_get_stat, ctx, ino, &stat);
	memcpy(bufstat, stat, sizeof(struct stat));
	kvsal_free(stat);
out:
	log_debug("ino=%d rc=%d", (int)bufstat->st_ino, rc);
	return rc;
}

int kvsns_setattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		  struct stat *setstat, int statflag)
{
	char k[KLEN];
	struct stat bufstat;
	struct timeval t;
	mode_t ifmt;

	if (!cred || !ino || !setstat)
		return -EINVAL;

	if (statflag == 0)
		return 0; /* Nothing to do */

	if (gettimeofday(&t, NULL) != 0)
		return -errno;

	RC_WRAP(kvsns_access, cred, ino, KVSNS_ACCESS_WRITE);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.stat", *ino);
	RC_WRAP(kvsal_get_stat, k, &bufstat);

	/* ctime is to be updated if md are changed */
	bufstat.st_ctim.tv_sec = t.tv_sec;
	bufstat.st_ctim.tv_nsec = 1000 * t.tv_usec;

	if (statflag & STAT_MODE_SET) {
		ifmt = bufstat.st_mode & S_IFMT;
		bufstat.st_mode = setstat->st_mode | ifmt;
	}

	if (statflag & STAT_UID_SET)
		bufstat.st_uid = setstat->st_uid;

	if (statflag & STAT_GID_SET)
		bufstat.st_gid = setstat->st_gid;

	if (statflag & STAT_SIZE_SET)
		RC_WRAP(extstore_truncate, ino, setstat->st_size, true,
			&bufstat);

	if (statflag & STAT_SIZE_ATTACH)
		RC_WRAP(extstore_truncate, ino, setstat->st_size, false,
			&bufstat);

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

	return kvsal_set_stat(k, &bufstat);
}

int kvsns2_setattr(kvsns_fs_ctx_t ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
		   struct stat *setstat, int statflag)
{
	struct stat bufstat;
	struct timeval t;
	mode_t ifmt;
	int rc;

	KVSNS_DASSERT(cred != NULL);
	KVSNS_DASSERT(setstat != NULL);
	KVSNS_DASSERT(ino != NULL);

	if (statflag == 0) {
		rc = 0;
		/* Nothing to do */
		goto out;
	}

	if (gettimeofday(&t, NULL) != 0) {
		rc = errno;
		return -errno;
	}

	RC_WRAP_LABEL(rc, out, kvsns2_access, ctx, cred, ino, KVSNS_ACCESS_WRITE);
	RC_WRAP_LABEL(rc, out, kvsns2_getattr, ctx, cred, ino, &bufstat);

	/* ctime is to be updated if md are changed */
	bufstat.st_ctim.tv_sec = t.tv_sec;
	bufstat.st_ctim.tv_nsec = 1000 * t.tv_usec;

	if (statflag & STAT_MODE_SET) {
		ifmt = bufstat.st_mode & S_IFMT;
		bufstat.st_mode = setstat->st_mode | ifmt;
	}

	if (statflag & STAT_UID_SET)
		bufstat.st_uid = setstat->st_uid;

	if (statflag & STAT_GID_SET)
		bufstat.st_gid = setstat->st_gid;

	/* @todo : Truncate is not implemented for for kvsns2 yet. */
	if (statflag & STAT_SIZE_SET)
		RC_WRAP_LABEL(rc, out, extstore_truncate, ino, setstat->st_size, true,
			      &bufstat);

	if (statflag & STAT_SIZE_ATTACH)
		RC_WRAP_LABEL(rc, out, extstore_truncate, ino, setstat->st_size, false,
			       &bufstat);

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
	RC_WRAP_LABEL(rc, out, kvsns2_set_stat, ctx, ino, &bufstat);
out:
	log_debug("rc=%d", rc);
	return rc;
}

int kvsns_link(kvsns_fs_ctx_t fs_ctx, kvsns_cred_t *cred, kvsns_ino_t *ino,
	       kvsns_ino_t *dino, char *dname)
{
	int rc;
	kvsns_ino_t tmpino = 0LL;
	kvsns_name_t k_name;

	KVSNS_DASSERT(cred && ino && dname && dino);

	log_trace("ENTER: ino=%llu dino=%llu dname=%s", *ino, *dino, dname);
	RC_WRAP(kvsal_begin_transaction);
	RC_WRAP_LABEL(rc, aborted, kvsns2_access, fs_ctx, cred, dino, KVSNS_ACCESS_WRITE);

	rc = kvsns2_lookup(fs_ctx, cred, dino, dname, &tmpino);
	if (rc == 0)
		return -EEXIST;

	RC_WRAP_LABEL(rc, aborted, kvsns_name_from_cstr, dname, &k_name);
	RC_WRAP_LABEL(rc, aborted, kvsns_tree_attach, fs_ctx, dino, ino, &k_name);

	RC_WRAP_LABEL(rc, aborted, kvsns2_update_stat, fs_ctx, ino,
		      STAT_CTIME_SET|STAT_INCR_LINK);

	log_trace("EXIT: rc=%d ino=%llu dino=%llu dname=%s", rc, *ino, *dino, dname);
	RC_WRAP(kvsal_end_transaction);
	return rc;
aborted:
	kvsal_discard_transaction();
	log_trace("EXIT: rc=%d ino=%llu dino=%llu dname=%s", rc, *ino, *dino, dname);
	return rc;
}

int kvsns_unlink(kvsns_cred_t *cred, kvsns_ino_t *dir, char *name)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino = 0LL;
	kvsns_ino_t parent[KVSAL_ARRAY_SIZE];
	struct stat ino_stat;
	struct stat dir_stat;
	int size;
	int i;
	bool opened;
	bool deleted;

	opened = false;
	deleted = false;

	if (!cred || !dir || !name)
		return -EINVAL;

	memset(parent, 0, KVSAL_ARRAY_SIZE*sizeof(kvsns_ino_t));
	memset(&ino_stat, 0, sizeof(ino_stat));
	memset(&dir_stat, 0, sizeof(dir_stat));

	RC_WRAP(kvsns_access, cred, dir, KVSNS_ACCESS_WRITE);

	RC_WRAP(kvsns_lookup, cred, dir, name, &ino);

	RC_WRAP(kvsns_get_stat, dir, &dir_stat);
	RC_WRAP(kvsns_get_stat, &ino, &ino_stat);

	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.parentdir", ino);
	RC_WRAP(kvsal_get_char, k, v);

	size = KVSAL_ARRAY_SIZE;
	RC_WRAP(kvsns_str2parentlist, parent, &size, v);

	/* Check if file is opened */
	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.openowner", ino);
	rc = kvsal_exists(k);
	if ((rc != 0) && (rc != -ENOENT))
		return rc;

	opened = (rc == -ENOENT) ? false : true;

	RC_WRAP(kvsal_begin_transaction);

	if (size == 1) {
		/* Last link, try to perform deletion */
	memset(k, 0, KLEN);
		snprintf(k, KLEN, "%llu.parentdir", ino);
		RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

		memset(k, 0, KLEN);
		snprintf(k, KLEN, "%llu.stat", ino);
		RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

		if (opened) {
			/* File is opened, deleted it at last close */
			memset(k, 0, KLEN);
			snprintf(k, KLEN, "%llu.opened_and_deleted", ino);
			snprintf(v, VLEN, "1");
			RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);
		}

		/* Remove all associated xattr */
		deleted = true;
	} else {
		for (i = 0; i < size ; i++)
			if (parent[i] == *dir) {
				/* In this list mgmt, setting value 0
				 * will make it ignored as str is rebuilt */
				parent[i] = 0;
				break;
			}
		memset(k, 0, KLEN);
		snprintf(k, KLEN, "%llu.parentdir", ino);
		RC_WRAP_LABEL(rc, aborted, kvsns_parentlist2str,
			      parent, size, v);
		RC_WRAP_LABEL(rc, aborted, kvsal_set_char, k, v);

		RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &ino_stat,
			 STAT_CTIME_SET|STAT_DECR_LINK);
		RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, &ino, &ino_stat);
	}


	memset(k, 0, KLEN);
	snprintf(k, KLEN, "%llu.dentries.%s",
		 *dir, name);
	RC_WRAP_LABEL(rc, aborted, kvsal_del, k);

	/* if object is a link, delete the link content as well */
	if (S_ISLNK(ino_stat.st_mode)) {
		memset(k, 0, KLEN);
		snprintf(k, KLEN, "%llu.link", ino);
		RC_WRAP_LABEL(rc, aborted, kvsal_del, k);
	}

	RC_WRAP_LABEL(rc, aborted, kvsns_amend_stat, &dir_stat,
		      STAT_MTIME_SET|STAT_CTIME_SET);
	RC_WRAP_LABEL(rc, aborted, kvsns_set_stat, dir, &dir_stat);

	RC_WRAP(kvsal_end_transaction);

	/* Call to object store : do not mix with metadata transaction */
	if (!opened)
		RC_WRAP(extstore_del, &ino);

	if (deleted)
		RC_WRAP(kvsns_remove_all_xattr, cred, &ino);
	return 0;

aborted:
	kvsal_discard_transaction();
	return rc;
}

int kvsns2_unlink(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *dir, char *name)
{
	int rc;
	char k[KLEN];
	char v[VLEN];
	kvsns_ino_t ino = 0LL;
	kvsns_ino_t parent[KVSAL_ARRAY_SIZE];
	struct stat *ino_stat = NULL;
	struct stat *dir_stat = NULL;
	size_t klen;
	size_t vlen;
	int size;
	bool opened;
	kvsns_fid_t kfid;
	kvsns_name_t k_name;

	opened = false;

	if (!cred || !dir || !name) {
		rc =  -EINVAL;
		goto aborted;
	}

	RC_WRAP_LABEL(rc, aborted, kvsns_name_from_cstr, name, &k_name);

	log_trace("ENTER: name=%s dir=%p", name, dir);

	memset(parent, 0, KVSAL_ARRAY_SIZE*sizeof(kvsns_ino_t));

	RC_WRAP(kvsns2_access, ctx, cred, dir, KVSNS_ACCESS_WRITE);

	RC_WRAP(kvsns2_lookup, ctx, cred, dir, name, &ino);

	RC_WRAP_LABEL(rc, dirfree, kvsns2_get_stat, ctx, dir, &dir_stat);
	RC_WRAP_LABEL(rc, errfree, kvsns2_get_stat, ctx, &ino, &ino_stat);

	/* Check if the file is opened */
	RC_WRAP_LABEL(rc, errfree, prepare_key, k, KLEN, "%llu.openowner.*", ino);
	rc = kvsal2_exists(ctx, k, KLEN);
	if ((rc != 0) && (rc != -ENOENT))
		goto aborted;

	opened = (rc == -ENOENT) ? false : true;

	RC_WRAP(kvsal_begin_transaction);

	RC_WRAP_LABEL(rc, errfree, kvsns_tree_detach, ctx, dir, &ino, &k_name);

	size = ino_stat->st_nlink;

	if (size == 1) {
		RC_WRAP_LABEL(rc, errfree, kvsns2_del_stat, ctx, &ino);

		if (opened) {
			/* File is opened, the final close will delete it */
			log_debug("File is opened by other thread/process.");
			RC_WRAP_LABEL(rc, errfree, prepare_key, k, KLEN,
				      "%llu.opened_and_deleted", ino);
			klen = rc;
			RC_WRAP_LABEL(rc, errfree, prepare_key, v, VLEN, "1");
			vlen = rc;
			RC_WRAP_LABEL(rc, errfree, kvsal2_set_char, ctx, k,
				      klen, v, vlen);
		}
		/* @todo: Remove all associated xattrs */
	}
	else if (size >= 1) {
	    ino_stat->st_nlink--;
	    RC_WRAP_LABEL(rc, errfree, kvsns2_set_stat, ctx, &ino, ino_stat);
	}

	/* if object is a link, delete the link content as well */
	/* @todo: Untested. Revisit this during implementation of symlinks. */
	if ((ino_stat->st_mode & S_IFLNK) == S_IFLNK)
		RC_WRAP_LABEL(rc, errfree, kvsns_del_link, ctx, &ino);

	RC_WRAP_LABEL(rc, errfree, kvsns_amend_stat, dir_stat,
		      STAT_MTIME_SET|STAT_CTIME_SET);
	RC_WRAP_LABEL(rc, errfree, kvsns2_set_stat, ctx, dir, dir_stat);

	RC_WRAP(kvsal_end_transaction);

	/* Call to object store : do not mix with metadata transaction */
	if (!opened && (ino_stat->st_mode & S_IFLNK) != S_IFLNK && (size == 1)) {
		RC_WRAP(extstore_ino_to_fid, ctx, ino, &kfid);
		RC_WRAP(extstore2_del, ctx, &ino, &kfid);
	}

errfree:
	kvsal_free(dir_stat);
dirfree:
	kvsal_free(ino_stat);
aborted:
	kvsal_discard_transaction();
	log_trace("EXIT rc=%d", rc);
	return rc;
}

int kvsns_rename(kvsns_fs_ctx_t fs_ctx, kvsns_cred_t *cred,
		 kvsns_ino_t *sino_dir, char *sname,
		 kvsns_ino_t *dino_dir, char *dname)
{
	int rc;
	bool overwrite_dst = false;
	bool rename_inplace = false;
	bool is_dst_non_empty_dir = false;
	kvsns_ino_t sino;
	kvsns_ino_t dino;
	struct stat *sstat = NULL;
	struct stat *dstat = NULL;
	kvsns_name_t k_sname;
	kvsns_name_t k_dname;

	KVSNS_DASSERT(cred);
	KVSNS_DASSERT(sino_dir && dino_dir);
	KVSNS_DASSERT(sname && dname);
	KVSNS_DASSERT(strlen(sname) <= NAME_MAX);
	KVSNS_DASSERT(strlen(dname) <= NAME_MAX);
	KVSNS_DASSERT((*sino_dir != *dino_dir || strcmp(sname, dname) != 0));
	KVSNS_DASSERT(fs_ctx);

	(void) kvsns_name_from_cstr(sname, &k_sname);
	(void) kvsns_name_from_cstr(dname, &k_dname);

	rename_inplace = (*sino_dir == *dino_dir);

	RC_WRAP_LABEL(rc, out, kvsns2_access, fs_ctx, cred, sino_dir,
		      KVSNS_ACCESS_DELETE_ENTITY);

	if (!rename_inplace) {
		RC_WRAP_LABEL(rc, out, kvsns2_access, fs_ctx, cred, dino_dir,
			      KVSNS_ACCESS_CREATE_ENTITY);
	}

	RC_WRAP_LABEL(rc, out, kvsns2_lookup, fs_ctx, cred, sino_dir, sname,
		      &sino);

	rc = kvsns2_lookup(fs_ctx, cred, dino_dir, dname, &dino);
	if (rc < 0 && rc != -ENOENT) {
		goto out;
	}
	overwrite_dst = (rc != -ENOENT);

	if (overwrite_dst) {
		RC_WRAP_LABEL(rc, sstatfree, kvsns2_get_stat, fs_ctx, &sino, &sstat);
		RC_WRAP_LABEL(rc, errfree, kvsns2_get_stat, fs_ctx, &dino, &dstat);
		if (S_ISDIR(sstat->st_mode) != S_ISDIR(dstat->st_mode)) {
			log_warn("Incompatible source and destination %d,%d.",
				 (int) sstat->st_mode, (int) dstat->st_mode);
			rc = -ENOTDIR;
			goto errfree;
		}
		if (S_ISDIR(dstat->st_mode)) {
			RC_WRAP_LABEL(rc, errfree, kvsns_tree_has_children, fs_ctx,
				      &dino, &is_dst_non_empty_dir);
		}
		if (is_dst_non_empty_dir) {
			log_warn("Destination is not empty (%llu:%s)",
				 dino, dname);
			rc = -EEXIST;
			goto errfree;
		}

		/* NOTE: We don't have transactions, so that let's just unlink()
		 * metadata an data right here.
		 * If we had transactions, tree_unlink() can be used here
		 * to remove metadata only, and then the actual data will be
		 * removed if the metadata operations were successfull, so that
		 * we have a chance to keep data as an orphaned inode if
		 * something goes wrong between end_transaction() and
		 * extstore_del().
		 */
		if (S_ISDIR(dstat->st_mode)) {
			/* @todo: replace with rmdir when it is implemented */
			RC_WRAP_LABEL(rc, errfree, kvsns_tree_detach, fs_ctx,
				      dino_dir, &dino, &k_dname);
		} else {
			RC_WRAP_LABEL(rc, errfree, kvsns2_unlink, fs_ctx, cred,
				      dino_dir, dname);
		}
	}

	if (rename_inplace) {
		/* a shortcut for renaming only a dentry
		 * witherrfree re-linking of the inodes.
		 */
		RC_WRAP_LABEL(rc, errfree, kvsns_tree_rename_link, fs_ctx,
			      sino_dir, &sino, &k_sname, &k_dname);
	} else {
		RC_WRAP_LABEL(rc, errfree, kvsns_tree_detach, fs_ctx, sino_dir,
			      &sino, &k_sname);
		RC_WRAP_LABEL(rc, errfree, kvsns_tree_attach, fs_ctx, dino_dir,
			      &sino, &k_dname);
	}

errfree:
	kvsal_free(dstat);
sstatfree:
	kvsal_free(sstat);
out:
	return rc;
}


int kvsns_mr_proper(void)
{
	int rc;
	char pattern[KLEN];
	kvsal_item_t items[KVSAL_ARRAY_SIZE];
	int i;
	int size;
	kvsal_list_t list;

	memset(pattern, 0, KLEN);
	snprintf(pattern, KLEN, "*");

	rc = kvsal_fetch_list(pattern, &list);
	if (rc < 0)
		return rc;

	do {
		size = KVSAL_ARRAY_SIZE;
		rc = kvsal_get_list(&list, 0, &size, items);
		if (rc < 0)
			return rc;

		for (i = 0; i < size ; i++)
			RC_WRAP(kvsal_del, items[i].str);

	} while (size > 0);

	rc = kvsal_fetch_list(pattern, &list);
	if (rc < 0)
		return rc;

	return 0;
}

int kvsns_fsid_to_ctx(kvsns_fsid_t fsid, kvsns_fs_ctx_t *fs_ctx)
{
	*fs_ctx = kvsns_fs_ctx;
	log_debug("fsid=%lu kvsns_fs_ctx=%p", fsid, *fs_ctx);
	return 0;
}

int kvsns_create_fs_ctx(kvsns_fsid_t fs_id, kvsns_fs_ctx_t *fs_ctx)
{
	RC_WRAP(kvsal_create_fs_ctx, fs_id, fs_ctx);

	kvsns_fs_ctx = *fs_ctx;
	return 0;
}
