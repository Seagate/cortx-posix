/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) Panasas Inc., 2011
 * Author: Jim Lieb jlieb@panasas.com
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
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

/* TODOs in this module:
	- TODO:SHARE_RESERVATION:
		Functions such as open2/close2/status2/setattr2/read2/write2
		which must check state of a FD and fsal_share state of
		a FH. Also, FDs and FHs must be protected with locks.
	- TODO:PERF:
		Performance improvements.
	- TODO:PORTING:
		Things which we will need to implement, but right now
		we can live without them. For instance, reopen2 which
		makes no sense without share reservation feature implemented.
*/

/* File States and File Handle.
 *  A file handle is the concept from NFSv3 where a file has a unique identifier
 *  used for various operations like READ/WRITE/LOOKUP etc. In our FSAL
 *  a file handle is just the inode of a file.
 *  NFSv4+ introduces a new concept of state (stateid) : the server side have to
 *  manage not only the filehandles within a filesystem, but also their states.
 *  In NFS Ganesha file states are represented by ::state_t structure while
 *  file handles are represented by ::fsal_obj_handle structure.
 *  Corresponddingly, the FSAL defines ::kvsfs_obj_handle and ::kvsfs_state_fd.
 *
 *  Right now KVSFS does not strore any information about file states in the
 *  KVS backend (i.e., does not call kvsns_open/close). Those calls will be
 *  implemented in the scope of the corresponding IO-related tickets.
 *
 *  File States and Open/Close implementation must be revisited when we add
 *  our own recovery engine.
 */

/******************************************************************************/
#include "kvsfs_methods.h"
#include "fsal_internal.h" /* kvsfs_fsal_module */
#include <gsh_list.h> /* container_of */
#include <fsal_convert.h> /* posix2fsal */
#include <FSAL/fsal_commonlib.h> /* FSAL methods */

/******************************************************************************/
/* Internal data types */

/** KVSFS-related data for a file state object. */
struct kvsfs_file_state {
	/** The open and share mode etc. */
	fsal_openflags_t openflags;

	/** The KVSNS file descriptor. */
	kvsns_file_open_t kvsns_fd;
};

/** KVSFS version of a file state object.
 * @see kvsfs_alloc_state and kvsfs_free_state.
 */
struct kvsfs_state_fd {
	/* base */
	struct state_t state;
	/* data */
	struct kvsfs_file_state kvsfs_fd;
};

/* Wrapper for a File Handle object. */
struct kvsfs_file_handle {
	kvsns_ino_t kvsfs_handle;
};

struct kvsfs_fsal_obj_handle {
	/* Base Handle */
	struct fsal_obj_handle obj_handle;

	/* KVSNS Handle */
	struct kvsfs_file_handle handle[1];

	/* TODO:PERF: Reduce memory consumption.
	 * kvsfs_fsal_export already has an open filesystem context,
	 * therefore we don't need to store it in each File Handle,
	 * unless we want to export multiple Clovis Indexes from a single
	 * export.
	 */
	/* KVSNS Context */
	struct kvsfs_fsal_index_context *fs_ctx;

	/* Global state is disabled because we don't support NFv3. */
	/* struct kvsfs_file_state global_fd; */

	/* Share reservations */
	struct fsal_share share;
};

/******************************************************************************/
/* Wrappers for KVSFS handle debug traces. Enable this "#if" if you want
 * to get traces even without enabled DEBUG logging level, i.e. if you want to
 * see only debug logs from this module.
 */
#if 1
#define T_ENTER(_fmt, ...) \
	LogCrit(COMPONENT_FSAL, "T_ENTER " _fmt, __VA_ARGS__)

#define T_EXIT(_fmt, ...) \
	LogCrit(COMPONENT_FSAL, "T_EXIT " _fmt, __VA_ARGS__)

#define T_TRACE(_fmt, ...) \
	LogCrit(COMPONENT_FSAL, "T_TRACE " _fmt, __VA_ARGS__)
#else
#define T_ENTER(_fmt, ...) \
	LogDebug(COMPONENT_FSAL, "T_ENTER " _fmt, __VA_ARGS__)

#define T_EXIT(_fmt, ...) \
	LogDebug(COMPONENT_FSAL, "T_EXIT " _fmt, __VA_ARGS__)

#define T_TRACE(_fmt, ...) \
	LogDebug(COMPONENT_FSAL, "T_TRACE " _fmt, __VA_ARGS__)
#endif

#define T_ENTER0 T_ENTER(">>> %s", "()");
#define T_EXIT0(__rcval)  T_EXIT("<<< rc=%d", __rcval);

/******************************************************************************/
/* Global variable imported from main.c */
extern struct kvsfs_fsal_module KVSFS;

/******************************************************************************/
static void construct_handle(struct fsal_export *export_base,
			     const kvsns_ino_t *ino,
			     const struct stat *stat,
			     struct fsal_obj_handle **obj)
{
	struct kvsfs_fsal_obj_handle *result;
	struct kvsfs_fsal_export *export =
	    container_of(op_ctx->fsal_export, struct kvsfs_fsal_export, export);

	result = gsh_calloc(1, sizeof(*result));

	result->handle[0] = (struct kvsfs_file_handle) { *ino };
	result->fs_ctx = export->index_context;

	fsal_obj_handle_init(&result->obj_handle,
			     export_base,
			     posix2fsal_type(stat->st_mode));

	result->obj_handle.fsid = posix2fsal_fsid(stat->st_dev);
	result->obj_handle.fileid = stat->st_ino;

	result->obj_handle.obj_ops = &KVSFS.handle_ops;

	*obj = &result->obj_handle;

	T_TRACE("Constructed handle: %p, INO: %d, FSID: %d:%d, FILEID: %d",
		*obj, (int) *ino,
		(int) result->obj_handle.fsid.major,
		(int) result->obj_handle.fsid.minor,
		(int) result->obj_handle.fileid);
}

/******************************************************************************/
/* EXPORT */
int kvsfs_export_to_kvsns_ctx(struct fsal_export *exp_hdl,
			      kvsns_fs_ctx_t *fs_ctx)
{
	(void) exp_hdl;
	int rc;
	kvsns_fsid_t fs_id = KVSNS_FS_ID_DEFAULT;
	kvsns_fs_ctx_t ctx = KVSNS_NULL_FS_CTX;

	rc = kvsns_fsid_to_ctx(fs_id, &ctx);
	if (rc != 0) {
		LogCrit(COMPONENT_FSAL,
			"fs_handle not found, fsid: %lu, rc: %d", fs_id, rc);
		return rc;
	}

	*fs_ctx = ctx;
	LogDebug(COMPONENT_FSAL, "kvsns_fs_ctx: %p ", *fs_ctx);
	return rc;
}

/******************************************************************************/
/* FSAL.lookup */
static fsal_status_t kvsfs_lookup(struct fsal_obj_handle *parent_hdl,
				  const char *name,
				  struct fsal_obj_handle **handle,
				  struct attrlist *attrs_out)
{
	struct kvsfs_fsal_obj_handle *parent, *hdl;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval;
	struct stat stat;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_ino_t object;
	struct kvsfs_fsal_export *export =
	    container_of(op_ctx->fsal_export, struct kvsfs_fsal_export, export);

	T_ENTER(">>> (%p, %s)", parent_hdl, name);

	assert(name);
	assert(strlen(name) > 0);

	if (!fsal_obj_handle_is(parent_hdl, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"Parent handle (%p) is not a directory.", parent_hdl);
		fsal_error = ERR_FSAL_NOTDIR;
		retval = -ENOTDIR;
		goto out;
	}

	parent = container_of(parent_hdl, struct kvsfs_fsal_obj_handle,
			     obj_handle);

	/* FIXME: kvsfs_lookup must be able to handle kookup(root_inode, "..")
	 * call and return root_inode for such a call.
	 */
	if (strcmp(name, "..") == 0 &&
	    parent->handle->kvsfs_handle == export->root_inode) {
		object = export->root_inode;
	} else {
		retval = kvsns2_lookup(parent->fs_ctx,
				       &cred,
				       &parent->handle->kvsfs_handle,
				       (char *) name, &object);
		if (retval < 0) {
			fsal_error = posix2fsal_error(-retval);
			goto out;
		}

	}

	retval = kvsns2_getattr(parent->fs_ctx, &cred, &object, &stat);
	if (retval < 0) {
		fsal_error = posix2fsal_error(-retval);
		goto out;
	}

	construct_handle(op_ctx->fsal_export, &object, &stat, handle);

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat, attrs_out);
	}

	fsal_error = ERR_FSAL_NO_ERROR;
	retval = 0;

 out:
	T_EXIT0(-retval);
	return fsalstat(fsal_error, -retval);
}

/******************************************************************************/
/* FSAL.looup_path - Find Inode of Export Root */
fsal_status_t kvsfs_lookup_path(struct fsal_export *exp_hdl,
			       const char *path,
			       struct fsal_obj_handle **handle,
			       struct attrlist *attrs_out)
{
	kvsns_ino_t object;
	int rc = 0;
	struct stat stat;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *hdl;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;
	struct kvsfs_fsal_export *myexport;

	T_ENTER0;
	assert(exp_hdl);

	myexport = container_of(op_ctx->fsal_export,
				struct kvsfs_fsal_export, export);

	/* Only the root inode is exported so far */
	if (strcmp(path, "/")) {
		return fsalstat(ERR_FSAL_NOTSUPP, 0);
	}

	object = myexport->root_inode;
	fs_ctx = myexport->index_context;

	rc = kvsns2_getattr(fs_ctx, &cred, &object, &stat);
	if (rc != 0) {
		return fsalstat(posix2fsal_error(-rc), -rc);
	}

	construct_handle(exp_hdl, &object, &stat, handle);

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat, attrs_out);
	}

	T_EXIT0(0);
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/******************************************************************************/
/* FSAL.mkdir - Create a directory */
static fsal_status_t kvsfs_mkdir(struct fsal_obj_handle *dir_hdl,
				const char *name, struct attrlist *attrs_in,
				struct fsal_obj_handle **handle,
				struct attrlist *attrs_out)
{
	struct kvsfs_fsal_obj_handle *myself, *hdl;
	int retval = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_ino_t object;
	struct stat stat;
	mode_t unix_mode;

	/* TODO:PERF: Check if it can be converted into an assert pre-cond */
	if (!fsal_obj_handle_is(dir_hdl, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"Parent handle is not a directory. hdl = 0x%p",
			dir_hdl);
		retval = -ENOTDIR;
		goto out;
	}

	myself = container_of(dir_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);

	unix_mode = fsal2unix_mode(attrs_in->mode)
		& ~op_ctx->fsal_export->exp_ops.fs_umask(op_ctx->fsal_export);

	retval = kvsns_mkdir(myself->fs_ctx, &cred,
			     &myself->handle->kvsfs_handle,
			     (char *) name, unix_mode, &object);
	if (retval < 0) {
		goto out;
	}

	retval = kvsns2_getattr(myself->fs_ctx, &cred, &object, &stat);
	if (retval < 0) {
		/* XXX:
		 * We have created a directory but cannot get its attributes
		 * What should we do here? remove it?
		 *
		 * TODO:PERF:
		 * kvsns_mkdir must return the stats of the created
		 * directory so that we can avoid this kvsns2_getattr call.
		 */
		goto out;
	}

	construct_handle(op_ctx->fsal_export, &object, &stat, handle);

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat, attrs_out);
	}

	retval = 0;
 out:
	return fsalstat(posix2fsal_error(-retval), -retval);
}

/******************************************************************************/
/** makesymlink
 *  Note that we do not set mode bits on symlinks for Linux/POSIX
 *  They are not really settable in the kernel and are not checked
 *  anyway (default is 0777) because open uses that target's mode.
 *  @see ::KVSNS_SYMLINK_MODE.
 */

static fsal_status_t kvsfs_makesymlink(struct fsal_obj_handle *dir_hdl,
				       const char *name, const char *link_path,
				       struct attrlist *attrib,
				       struct fsal_obj_handle **handle,
				       struct attrlist *attrs_out)
{
	struct kvsfs_fsal_obj_handle *myself, *hdl;
	int retval = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_ino_t object;
	struct stat stat;
	struct fsal_obj_handle *new_hdl = NULL;
	fsal_status_t status = fsalstat(ERR_FSAL_NO_ERROR, 0);

	if (!fsal_obj_handle_is(dir_hdl, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"Parent handle is not a directory. hdl = 0x%p",
			dir_hdl);
		status = fsalstat(ERR_FSAL_NOTDIR, ENOTDIR);
		goto out;
	}

	myself = container_of(dir_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);

	retval = kvsns_symlink(myself->fs_ctx, &cred,
			       &myself->handle->kvsfs_handle,
			       (char *)name, (char *)link_path, &object);
	if (retval < 0) {
		status = fsalstat(posix2fsal_error(-retval), -retval);
		goto out;
	}

	retval = kvsns2_getattr(myself->fs_ctx, &cred, &object, &stat);
	if (retval < 0) {
		status = fsalstat(posix2fsal_error(-retval), -retval);
		goto out;
	}

	construct_handle(op_ctx->fsal_export, &object, &stat, &new_hdl);

	/* The caller might want to set something except 'mode' */
	if (FSAL_TEST_MASK(attrib->valid_mask, ATTR_MODE)) {
		FSAL_UNSET_MASK(attrib->valid_mask, ATTR_MODE);

		if (attrib->valid_mask) {
			status = new_hdl->obj_ops->setattr2(new_hdl, false,
							    NULL, attrib);
			if (FSAL_IS_ERROR(status)) {
				/* Revert attrib back and go out */
				LogCrit(COMPONENT_FSAL,
					"Failed to setattr2, status=%s",
					fsal_err_txt(status));
				FSAL_SET_MASK(attrib->valid_mask, ATTR_MODE);
				goto out;
			}
		} else {
			status = fsalstat(ERR_FSAL_NO_ERROR, 0);

			if (attrs_out != NULL) {
				/* Since we haven't set any attributes other than what
				 * was set on create, just use the stat results we used
				 * to create the fsal_obj_handle.
				 */
				posix2fsal_attributes_all(&stat, attrs_out);
			}
		}

		FSAL_SET_MASK(attrib->valid_mask, ATTR_MODE);
	}

	/* Hand it over to the caller */
	*handle = new_hdl;
	new_hdl = NULL;

out:
	/* Release the handle if we haven't transfered it to the caller */
	if (new_hdl) {
		new_hdl->obj_ops->release(new_hdl);
	}

	return status;
}

/******************************************************************************/
/* FSAL.realink */
static fsal_status_t kvsfs_readsymlink(struct fsal_obj_handle *obj_hdl,
				      struct gsh_buffdesc *link_content,
				      bool refresh)
{
	struct kvsfs_fsal_obj_handle *myself = NULL;
	int retval = 0;
	int retlink = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	if (!fsal_obj_handle_is(obj_hdl, SYMBOLIC_LINK)) {
		fsal_error = ERR_FSAL_FAULT;
		goto out;
	}

	myself = container_of(obj_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);

	link_content->len = fsal_default_linksize;
	link_content->addr = gsh_malloc(link_content->len);

	retlink = kvsns_readlink(myself->fs_ctx, &cred,
				 &myself->handle->kvsfs_handle,
				 link_content->addr, &link_content->len);

	if (retlink < 0) {
		fsal_error = posix2fsal_error(-retlink);
		gsh_free(link_content->addr);
		link_content->addr = NULL;
		link_content->len = 0;
		goto out;
	}

	link_content->len = strlen(link_content->addr) + 1;

 out:
	return fsalstat(fsal_error, -retval);
}

/******************************************************************************/
/* FSAL.link */
static fsal_status_t kvsfs_linkfile(struct fsal_obj_handle *obj_hdl,
				    struct fsal_obj_handle *destdir_hdl,
				    const char *name)
{
	struct kvsfs_fsal_obj_handle *myself;
	struct kvsfs_fsal_obj_handle *destdir;
	int retval = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	myself = container_of(obj_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);

	destdir = container_of(destdir_hdl, struct kvsfs_fsal_obj_handle,
			       obj_handle);

	retval = kvsns_link(destdir->fs_ctx, &cred,
			    &myself->handle->kvsfs_handle,
			    &destdir->handle->kvsfs_handle,
			    (char *) name);

	return fsalstat(posix2fsal_error(-retval), -retval);
}

/******************************************************************************/
/* FSAL.readdir */

/** State (context) for kvsfs_readdir_cb.
 * The callback is called when the current postion ("where") >= the start
 * position ("whence").
 * eof and dir_continue flags are cleared when iteractions were iterrupted
 * by the user (nfs-ganesha).
 * */
struct kvsfs_readdir_cb_ctx {
	/** READDIR starts from offset specified by "whence". */
	const fsal_cookie_t whence;
	/** nfs-ganesha callback which is called for each dir entry. */
	const fsal_readdir_cb fsal_cb;
	/** Current offset (index of "where" we are right now). */
	fsal_cookie_t where;
	/** Context for the nfs-ganesha callback. */
	void *fsal_cb_ctx;
	/** The flag is set to False when nfs-ganesha requested termination
	 * of readdir procedure.
	 */
	bool dir_continue;
	/** Flag is set to False when readdir was terminated and didn't
	 * reach the last dir entry.
	 */
	bool eof;

	attrmask_t attrmask;
	int inner_rc;

	struct kvsfs_fsal_obj_handle *parent;
};

#define KVSFS_FIRST_COOKIE 3

/** A callback to be called for each READDIR entry.
 * @param[in, out] ctx Callback state (context).
 * @param[in] name Name of the dentry.
 * @param[in] ino Inode of the dentry.
 * @retval true if more entries are requested.
 * @retval false if iterations must be interrupted.
 * @see populate_dirent in nfs-ganesha.
 * NOTE: "ino" is unused but it is reserved for futher migrations
 * to the recent nfs-ganesha. It will allow us to avoid lookup() call and
 * directly call getattr().
*/
static bool kvsfs_readdir_cb(void *ctx, const char *name,
			     const kvsns_ino_t *ino)
{
	struct kvsfs_readdir_cb_ctx *cb_ctx = ctx;
	enum fsal_dir_result dir_res;
	struct attrlist attrs;
	struct fsal_obj_handle *obj;

	assert(cb_ctx != NULL);
	assert(name != NULL);
	assert(ino != NULL);

	T_ENTER("dentry[%d]=%s, ino=%d", (int) cb_ctx->where, name, (int) *ino);
	/* A small state machine for dir_continue and eof logic:
	 * even if cb_ctx->fsal_cb returned false, we still have to
	 * check if it was the last dir entry.
	 * TODO:PERF: Add "unlikely" wrapper here.
	 */
	if (!cb_ctx->dir_continue) {
		cb_ctx->eof = false;
		T_EXIT("%s", "USER_CANCELED");
		return false;
	}

	if (cb_ctx->where < cb_ctx->whence) {
		T_EXIT("%s", "SKIP");
		return true;
	}

	fsal_prepare_attrs(&attrs, cb_ctx->attrmask);

	/* TODO:PORTING: move into a separate function to obtain attrs from
	 * kvsns
	 */
	{
		kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
		struct stat stat;
		int retval;
		kvsns_ino_t object = *ino;

		retval = kvsns2_getattr(cb_ctx->parent->fs_ctx, &cred, &object,
					&stat);
		if (retval < 0) {
			cb_ctx->inner_rc = retval;
			goto err_out;
		}

		construct_handle(op_ctx->fsal_export, &object, &stat, &obj);
		posix2fsal_attributes_all(&stat, &attrs);
	}

	T_TRACE("READDIR_CB: %s, %p, %d", name, obj, (int) cb_ctx->where);

	dir_res = cb_ctx->fsal_cb(name, obj, &attrs, cb_ctx->fsal_cb_ctx,
				  cb_ctx->where);

	fsal_release_attrs(&attrs);

	cb_ctx->dir_continue = dir_res == DIR_CONTINUE;

	cb_ctx->where++;

	T_EXIT("NEED_NEXT %d, %d", (int) cb_ctx->dir_continue, (int) dir_res);
	return true;

err_out:
	T_EXIT("CANCEL_ERROR %d", (int) cb_ctx->inner_rc);
	return false;
}

static fsal_status_t kvsfs_readdir(struct fsal_obj_handle *dir_hdl,
				  fsal_cookie_t *whence, void *dir_state,
				  fsal_readdir_cb cb, attrmask_t attrmask,
				  bool *eof)

{
	struct kvsfs_fsal_obj_handle *obj;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_readdir_cb_ctx readdir_ctx = {
		.whence = whence ? *whence : KVSFS_FIRST_COOKIE,
		.where = KVSFS_FIRST_COOKIE,
		.fsal_cb = cb,
		.fsal_cb_ctx = dir_state,
		.eof = true,
		.dir_continue = true,
		.attrmask = attrmask,
		.parent = NULL,
		.inner_rc = 0,
	};
	kvsns_fs_ctx_t fs_ctx;
	int rc;

	obj = container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	T_ENTER("parent=%d", (int) obj->handle->kvsfs_handle);

	fs_ctx = obj->fs_ctx;
	readdir_ctx.parent = obj;

	/* NOTE:PERF:
	 *	The current version of nfs-ganesha calls fsal_readdir()
	 *	to update its internal inode cache. It fills the cache,
	 *	and only after that returns a response to the client.
	 *	Also, it calls lookup() and getattr() for each entry.
	 *	Moreover, nfs-ganesha never uses whernce != 0.
	 *	Considering the above statements, there is no need to
	 *	use the previous opendir/readdir-by-offset/closedir
	 *	approach -- we can freely just walk over the whole directory.
	 */
	/* NOTE: nfs-ganesha migration.
	 *	The recent version of nfs-ganesha has a mdcache FSAL instead
	 *	of the inode cache. It looks like the MD FSAL is able to update
	 *	only parts (chunks) of readdir contents depending on
	 *	the state of L1 LRU and L2 MRU caches.
	 *	Moreover, the new version supports a whence-as-name export
	 *	option which allows to use filenames as offsets (see RGW FSAL).
	 *	Therefore, the readdir kvsnsn interface can be extened
	 *	to support a start dir entry name as an option.
	 *	Also, the new version will requite readdir_cb to call
	 *	getattr().
	 */
	rc = kvsns_readdir(fs_ctx, &cred, &obj->handle->kvsfs_handle,
			   kvsfs_readdir_cb, &readdir_ctx);

	if (rc < 0)
		goto out;

	if (readdir_ctx.inner_rc != 0 ) {
		rc = readdir_ctx.inner_rc;
		goto out;
	}

	*eof = readdir_ctx.eof;

 out:
	T_EXIT0(-rc);
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/******************************************************************************/
/* FSAL.rename */
/** Rename (re-link) an object in a filesystem.
 *  @param[in] obj_hdl An existing object in the FS to be renamed.
 *  @param[in] olddir_hdl A parent dir where the object is linked under the
 *  name `old_name`.
 *  @param[in] old_name The current name of the object.
 *  @param[in] newdir_hdl A dir where the object will be linked under the name
 *  `new_name`.
 *  @param[in] new_name A name of the object.
 *  @return @see kvsns2_rename error codes.
 */
static fsal_status_t kvsfs_rename(struct fsal_obj_handle *obj_hdl,
				 struct fsal_obj_handle *olddir_hdl,
				 const char *old_name,
				 struct fsal_obj_handle *newdir_hdl,
				 const char *new_name)
{
	struct kvsfs_fsal_obj_handle *olddir;
	struct kvsfs_fsal_obj_handle *newdir;
	struct kvsfs_fsal_obj_handle *obj;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int rc = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	olddir = container_of(olddir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	newdir = container_of(newdir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

#if 1
	/* TODO:PERF: Consider replacing with assert */
	if (obj->fs_ctx != olddir->fs_ctx) {
		LogFatal(COMPONENT_FSAL,
			 "obj fs_ctx does not match olddir fs_ctx "
			 "'%p' != '%p'", obj_hdl, olddir_hdl);
		rc = -EXDEV;
		fsal_error = ERR_FSAL_XDEV;
		goto out;
	}
	if (obj->fs_ctx != newdir->fs_ctx) {
		LogFatal(COMPONENT_FSAL,
			 "obj fs_ctx does not match newdir fs_ctx "
			 "'%p' != '%p'", obj_hdl, newdir_hdl);
		rc = -EXDEV;
		fsal_error = ERR_FSAL_XDEV;
		goto out;
	}
#endif

	/* TODO:PERF: obj can be passed down in order to eliminate one extra
	 * `lookup` call, but the inode of the object is not a part
	 * of the kvsns_rename API right now.
	 */
	rc = kvsns_rename(obj->fs_ctx, &cred,
			  &olddir->handle->kvsfs_handle, (char *) old_name,
			  &newdir->handle->kvsfs_handle, (char *) new_name);
	if (rc != 0) {
		goto out;
	}

out:
	fsal_error = posix2fsal_error(-rc);
	return fsalstat(fsal_error, -rc);

}

/******************************************************************************/
/* FSAL.getattr */
static fsal_status_t kvsfs_getattrs(struct fsal_obj_handle *obj_hdl,
				    struct attrlist *attrs_out)
{
	struct kvsfs_fsal_obj_handle *myself;
	struct stat stat;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int retval = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	T_ENTER(">>> (%p)", obj_hdl);

	myself =
		container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	retval = kvsns2_getattr(myself->fs_ctx, &cred,
				&myself->handle->kvsfs_handle, &stat);
	if (retval < 0) {
		goto out;
	}

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat, attrs_out);
	}

out:
	fsal_error = posix2fsal_error(-retval);
	T_EXIT0(-retval);
	return fsalstat(fsal_error, -retval);

#if 0
	/* TODO:PORTING:
	 * Since we don't support NFSv3, there is no sense to support
	 * "stateless" NFSv3 file states (oxymoron).
	 * NFS Ganesha and their state_t-related functions are
	 * responsible for situations like the below case.
	 */
	/* An explanation is required here.
	 * This is an exception management.
	 * when a file is opened, then deleted without being closed,
	 * FSAL_VFS can still getattr on it, because it uses fstat
	 * on a cached FD. This is not possible
	 * to do this with KVSFS, because you can't fstat on a vnode.
	 * To handle this, stat are
	 * cached as the file is opened and used here,
	 * to emulate a successful fstat */
	if ((retval == ENOENT)
	    && (myself->u.file.openflags != FSAL_O_CLOSED)
	    && (S_ISREG(myself->u.file.saved_stat.st_mode))) {
		retval = 0;	/* remove the error */
		goto ok_file_opened_and_deleted;
	}

	if (retval)
		goto errout;

	/* convert attributes */
 ok_file_opened_and_deleted:
	posix2fsal_attributes(&stat, &myself->attributes);
	goto out;

 errout:
	if (retval == ENOENT)
		fsal_error = ERR_FSAL_STALE;
	else
		fsal_error = posix2fsal_error(-retval);
 out:
#endif
}

/******************************************************************************/
/* FSAL.getattr */

/* a jury-rigged convertor from NFS attrs into POSIX stats */
static fsal_status_t kvsfs_setattr_attrlist2stat(struct fsal_obj_handle *obj_hdl,
						 struct attrlist *attrs,
						 struct stat *stats_out,
						 int *flags_out)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct stat stats = { 0};
	int flags = 0;

	/* apply umask, if mode attribute is to be changed */
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_MODE))
		attrs->mode &= ~op_ctx->fsal_export->exp_ops.
				fs_umask(op_ctx->fsal_export);

	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_SIZE)) {
		if (obj_hdl->type != REGULAR_FILE) {
			fsal_error = ERR_FSAL_INVAL;
			return fsalstat(fsal_error, retval);
		}
		flags |= STAT_SIZE_SET;
		stats.st_size = attrs->filesize;
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_MODE)) {
		flags |= STAT_MODE_SET;
		stats.st_mode = fsal2unix_mode(attrs->mode);
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_OWNER)) {
		flags |= STAT_UID_SET;
		stats.st_uid = attrs->owner;
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_GROUP)) {
		flags |= STAT_GID_SET;
		stats.st_gid = attrs->group;
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_ATIME)) {
		flags |= STAT_ATIME_SET;
		stats.st_atime = attrs->atime.tv_sec;
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_ATIME_SERVER)) {
		flags |= STAT_ATIME_SET;
		struct timespec timestamp;

		retval = clock_gettime(CLOCK_REALTIME, &timestamp);
		if (retval != 0)
			goto out;
		stats.st_atim = timestamp;
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_MTIME)) {
		flags |= STAT_MTIME_SET;
		stats.st_mtime = attrs->mtime.tv_sec;
	}
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_MTIME_SERVER)) {
		flags |= STAT_MTIME_SET;
		struct timespec timestamp;

		retval = clock_gettime(CLOCK_REALTIME, &timestamp);
		if (retval != 0)
			goto out;
		stats.st_mtim = timestamp;
	}

out:
	if (retval == 0) {
		*flags_out = flags;
		*stats_out = stats;
		return fsalstat(ERR_FSAL_NO_ERROR, 0);
	}

	/* Exit with an error */
	fsal_error = posix2fsal_error(-retval);
	return fsalstat(fsal_error, -retval);
}

/* Atomicaly modifies the size of a file and its stats.
 * The function opens a WRITE share reservation, checks it
 * and then updates the file size and its attibutes
 * witin a single transaction.
 * @see https://tools.ietf.org/html/rfc7530#section-16.32.
 * TODO: Not implemented yet.
 */
static fsal_status_t
kvsfs_ftruncate(struct kvsfs_fsal_obj_handle *obj,
		struct state_t *state,
		bool bypass,
		struct stat *prev_stat,
		struct stat *new_stat,
		int new_stat_flags)
{
	(void) obj;
	(void) state;
	(void) bypass;
	(void) prev_stat;
	(void) new_stat;
	(void) new_stat_flags;

	T_ENTER0;
	T_EXIT0(0);
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

fsal_status_t kvsfs_setattrs(struct fsal_obj_handle *obj_hdl,
			     bool bypass,
			     struct state_t *state,
			     struct attrlist *attrs)
{
	fsal_status_t result;
	int rc;
	struct kvsfs_fsal_obj_handle *obj;
	struct stat stats = { 0 };
	int flags = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	result = kvsfs_setattr_attrlist2stat(obj_hdl, attrs, &stats, &flags);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_SIZE)) {
		if (obj_hdl->type != REGULAR_FILE) {
			LogFullDebug(COMPONENT_FSAL,
				"Setting size on non-regular file");
			result = fsalstat(ERR_FSAL_INVAL, EINVAL);
			goto out;
		}

		/* Truncate is a special IO-related operation */
		result = kvsfs_ftruncate(obj, state, bypass, NULL, &stats,
					 flags);
	} else {
		/* If the size does not need to be change, then
		 * we can simply update the stats associated with the inode
		 */
		rc = kvsns2_setattr(obj->fs_ctx, &cred,
				    &obj->handle->kvsfs_handle,
				    &stats, flags);
		result = fsalstat(posix2fsal_error(-rc), -rc);
	}

out:
	return result;
}

/******************************************************************************/
/* FSAL.unlink - Unlink a file or a directory. */
static fsal_status_t kvsfs_unlink(struct fsal_obj_handle *dir_hdl,
				  struct fsal_obj_handle *obj_hdl,
				  const char *name)
{
	struct kvsfs_fsal_obj_handle *parent;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	int rc = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;

	parent =
		container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	/* TODO:PERF:
	 * Pass obj->handle->kvsfs_handle instead of 'name'
	 * to avoid extra lookup() call in the KVSNS layer.
	 */
	if (fsal_obj_handle_is(obj_hdl, DIRECTORY)) {
		rc = kvsns2_rmdir(parent->fs_ctx, &cred,
				  &parent->handle->kvsfs_handle,
				  (char *) name);
	} else {
		/* KVSNS is able to create a regular file or a symlink,
		 * therefore it can unlink only these types of files
		 */
		assert(fsal_obj_handle_is(obj_hdl, REGULAR_FILE) ||
		       fsal_obj_handle_is(obj_hdl, SYMBOLIC_LINK));
		rc = kvsns2_unlink(parent->fs_ctx, &cred,
				   &parent->handle->kvsfs_handle,
				   (char *) name);
	}

	if (rc != 0) {
		LogCrit(COMPONENT_FSAL, "Delete failed obj=%s, rc=%d",
			name, rc);
		goto out;
	}

out:
	fsal_error = posix2fsal_error(-rc);
	return fsalstat(fsal_error, -rc);
}

/******************************************************************************/
/* FSAL.handle_to_wire */

/* handle_digest
 * fill in the opaque f/s file handle part.
 * we zero the buffer to length first.  This MAY already be done above
 * at which point, remove memset here because the caller is zeroing
 * the whole struct.
 */
static fsal_status_t kvsfs_handle_digest(const struct fsal_obj_handle *obj_hdl,
					fsal_digesttype_t output_type,
					struct gsh_buffdesc *fh_desc)
{
	const struct kvsfs_fsal_obj_handle *myself;
	const struct kvsfs_file_handle *fh;
	const size_t fh_size = sizeof(fh);

	assert(fh_desc);
	assert(obj_hdl);

	if (fh_desc->len < fh_size) {
		LogMajor(COMPONENT_FSAL,
			 "Space too small for handle.  need %lu, have %lu",
			 fh_size, fh_desc->len);
		return fsalstat(ERR_FSAL_TOOSMALL, 0);
	}

	myself =
	    container_of(obj_hdl, const struct kvsfs_fsal_obj_handle,
			 obj_handle);

	fh = myself->handle;

	switch (output_type) {
	case FSAL_DIGEST_NFSV3:
	case FSAL_DIGEST_NFSV4:
		memcpy(fh_desc->addr, fh, fh_size);
		break;
	default:
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}

	fh_desc->len = fh_size;
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/******************************************************************************/
/* FSAL_EXPORT.wite_to_host */

/* extract a file handle from a buffer.
 * do verification checks and flag any and all suspicious bits.
 * Return an updated fh_desc into whatever was passed.  The most
 * common behavior, done here is to just reset the length.  There
 * is the option to also adjust the start pointer.
 */
fsal_status_t kvsfs_extract_handle(struct fsal_export *exp_hdl,
					 fsal_digesttype_t in_type,
					 struct gsh_buffdesc *fh_desc,
					 int flags)
{
	const size_t fh_size = sizeof(struct kvsfs_file_handle);

	assert(fh_desc);
	assert(fh_desc->addr);

	if (in_type != FSAL_DIGEST_NFSV3 && in_type != FSAL_DIGEST_NFSV4) {
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}

	/* TODO:PORTING
	 * Figure out what exactly should we do here -
	 *	1. check the size
	 *	2. assign the size
	 * Together it looks very strange.
	 */
	if (fh_desc->len != fh_size) {
		LogMajor(COMPONENT_FSAL,
			 "Size mismatch for handle.  should be %lu, got %u",
			 (unsigned long int)fh_size,
			 (unsigned int)fh_desc->len);
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}

	fh_desc->len = fh_size;	/* pass back the actual size */
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/******************************************************************************/
/* FSAL.release - Release a File Handle */
static void release(struct fsal_obj_handle *obj_hdl)
{
	struct kvsfs_fsal_obj_handle *obj;

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	fsal_obj_handle_fini(obj_hdl);

	gsh_free(obj);
}

/******************************************************************************/
/* FSAL.handle_to_key */

/*
 * return a handle descriptor into the handle in this object handle
 * @TODO reminder.  make sure things like hash keys don't point here
 * after the handle is released.
 */

static void kvsfs_handle_to_key(struct fsal_obj_handle *obj_hdl,
				struct gsh_buffdesc *fh_desc)
{
	struct kvsfs_fsal_obj_handle *myself;

	myself = container_of(obj_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);
	fh_desc->addr = myself->handle;
	fh_desc->len = sizeof(myself->handle);
}

/******************************************************************************/
/* FSAL_EXPORT.create_handle */

/* Create handle for export */
fsal_status_t kvsfs_create_handle(struct fsal_export *exp_hdl,
				  struct gsh_buffdesc *hdl_desc,
				  struct fsal_obj_handle **handle,
				  struct attrlist *attrs_out)
{
	struct kvsfs_fsal_obj_handle *hdl;
	struct kvsfs_file_handle *fh;
	struct kvsfs_fsal_export *myexport;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct stat stat;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	int retval;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;

	assert(exp_hdl);
	assert(hdl_desc);
	assert(hdl_desc->addr);
	assert(handle);

	myexport = container_of(op_ctx->fsal_export,
				struct kvsfs_fsal_export, export);

	if (hdl_desc->len != sizeof(struct kvsfs_file_handle)) {
		fsal_error = ERR_FSAL_FAULT;
		retval = 0;
		goto out;
	}

	fh = hdl_desc->addr;

	retval = kvsns2_getattr(myexport->index_context, &cred,
				&fh->kvsfs_handle, &stat);
	if (retval < 0) {
		fsal_error = posix2fsal_error(-retval);
		goto out;
	}

	construct_handle(exp_hdl, &fh->kvsfs_handle, &stat, handle);

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat, attrs_out);
	}

	/* TODO:PERF:
	 * We can check the FH is a symlink and fetch its contents
	 * into the FH descriptor (we even know its size here) but
	 * we don't know yet what to do this the case where two
	 * NFS Ganesha instances operates on the same KVSNS filesytem.
	 * So that let's keep the conents uncached.
	 */
out:
	return fsalstat(fsal_error, -retval);
}


/******************************************************************************/
/** The function is trying to apply the new openflags
 * and returns the corresponding error if a share conflict
 * has been detected.
 */
static fsal_status_t kvsfs_share_try_new_state(struct kvsfs_fsal_obj_handle *obj,
					       fsal_openflags_t old_openflags,
					       fsal_openflags_t new_openflags)
{
	fsal_status_t status;

	PTHREAD_RWLOCK_wrlock(&obj->obj_handle.obj_lock);

	status = check_share_conflict(&obj->share, new_openflags, false);
	if (FSAL_IS_ERROR(status)) {
		goto out;
	}

	update_share_counters(&obj->share, old_openflags, new_openflags);

out:
	PTHREAD_RWLOCK_unlock(&obj->obj_handle.obj_lock);
	return status;
}

/* Unconditionally applies the new share reservations state. */
static void kvsfs_share_set_new_state(struct kvsfs_fsal_obj_handle *obj,
				      fsal_openflags_t old_openflags,
				      fsal_openflags_t new_openflags)
{
	PTHREAD_RWLOCK_wrlock(&obj->obj_handle.obj_lock);

	update_share_counters(&obj->share, old_openflags, new_openflags);

	PTHREAD_RWLOCK_unlock(&obj->obj_handle.obj_lock);
}

/******************************************************************************/
static const struct kvsfs_file_handle kvsfs_invalid_file_handle = { 0 };
static inline
bool kvsfs_file_state_invariant_closed(const struct kvsfs_file_state *state)
{
	return (state->openflags == FSAL_O_CLOSED) &&
		(state->kvsns_fd.ino == kvsfs_invalid_file_handle.kvsfs_handle);
}

static inline
bool kvsfs_file_state_invariant_open(const struct kvsfs_file_state *state)
{
	return (state->openflags != FSAL_O_CLOSED) &&
		(state->kvsns_fd.ino != kvsfs_invalid_file_handle.kvsfs_handle);
}

/******************************************************************************/
/* A wrapper for an OPEN-like call at the kvsns layer which stores
 * a file state into the KVS.
 */
static fsal_status_t kvsns_file_open(struct kvsfs_file_state *state,
				     fsal_openflags_t openflags,
				     struct kvsfs_fsal_obj_handle *obj)
{
	(void) state;
	(void) openflags;
	(void) obj;
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* A wrapper for a CLOSE-like call at the kvsns layer which lets the KVS
 * know that we don't need to keep the file open anymore.
 * @see kvsns_file_open.
 */
static fsal_status_t kvsns_file_close(struct kvsfs_file_state *state,
				      struct kvsfs_fsal_obj_handle *obj)
{
	(void) state;
	(void) obj;
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/******************************************************************************/
/* Opens and re-opens a file handle and creates (or re-uses) a file state
 * checking share reservations and propogating open call down into kvsns.
 */
static fsal_status_t kvsfs_file_state_open(struct kvsfs_file_state *state,
					   const fsal_openflags_t openflags,
					   struct kvsfs_fsal_obj_handle *obj,
					   bool is_reopen)
{
	fsal_status_t status;

	if (!is_reopen) {
		/* we cannot open the same state twice unless it is a reopen*/
		assert(kvsfs_file_state_invariant_closed(state));
	}

	status = kvsfs_share_try_new_state(obj, state->openflags, openflags);
	if (FSAL_IS_ERROR(status)) {
		goto out;
	}

	status = kvsns_file_open(state, openflags, obj);
	if (FSAL_IS_ERROR(status)) {
		goto undo_share;
	}

	state->openflags = openflags;
	state->kvsns_fd.ino = obj->handle->kvsfs_handle;

	assert(kvsfs_file_state_invariant_open(state));
	status = fsalstat(ERR_FSAL_NO_ERROR, 0);

undo_share:
	if (FSAL_IS_ERROR(status)) {
		kvsfs_share_set_new_state(obj, openflags, state->openflags);
	}

out:
	/* On exit it has to either remain closed (on error) or
	 * be in an open state (success).
	 */
	if (FSAL_IS_ERROR(status) && !is_reopen) {
		assert(kvsfs_file_state_invariant_closed(state));
	} else {
		assert(kvsfs_file_state_invariant_open(state));
	}

	return status;
}

/******************************************************************************/
/* Closes a file state associate with a file handle. */
static fsal_status_t kvsfs_file_state_close(struct kvsfs_file_state *state,
					    struct kvsfs_fsal_obj_handle *obj)
{
	fsal_status_t status;

	assert(state != NULL);
	assert(kvsfs_file_state_invariant_open(state));

	status = kvsns_file_close(state, obj);
	if (FSAL_IS_ERROR(status)) {
		goto out;
	}

	kvsfs_share_set_new_state(obj, state->openflags, FSAL_O_CLOSED);

	state->openflags = FSAL_O_CLOSED;
	state->kvsns_fd.ino = kvsfs_invalid_file_handle.kvsfs_handle;

out:
	/* We cannot guarantee that the file is always closed,
	 * but at least we can assume that the succesfull result always
	 * leads to the closed state
	 */
	if (!FSAL_IS_ERROR(status)) {
		assert(kvsfs_file_state_invariant_closed(state));
	}

	return status;
}

/******************************************************************************/
/* FSAL_EXPORT.alloc_state */
struct state_t *kvsfs_alloc_state(struct fsal_export *exp_hdl,
				enum state_type state_type,
				struct state_t *related_state)
{
	struct kvsfs_state_fd *super;

	super = gsh_calloc(1, sizeof(struct kvsfs_state_fd));

	super->kvsfs_fd.openflags = FSAL_O_CLOSED;
	super->kvsfs_fd.kvsns_fd.ino = kvsfs_invalid_file_handle.kvsfs_handle;

	return init_state(&super->state, exp_hdl, state_type, related_state);
}

/******************************************************************************/
/* FSAL_EXPORT.free_state */
void kvsfs_free_state(struct fsal_export *exp_hdl, struct state_t *state)
{
	struct kvsfs_state_fd *super;

	(void) exp_hdl;

	super = container_of(state, struct kvsfs_state_fd, state);

	gsh_free(super);
}

/******************************************************************************/
/* FSAL.open2 */
#if 0
/* Copied from fsal API */
/**
 * @brief Open a file descriptor for read or write and possibly create
 *
 * This function opens a file for read or write, possibly creating it.
 * If the caller is passing a state, it must hold the state_lock
 * exclusive.
 *
 * state can be NULL which indicates a stateless open (such as via the
 * NFS v3 CREATE operation), in which case the FSAL must assure protection
 * of any resources. If the file is being created, such protection is
 * simple since no one else will have access to the object yet, however,
 * in the case of an exclusive create, the common resources may still need
 * protection.
 *
 * If Name is NULL, obj_hdl is the file itself, otherwise obj_hdl is the
 * parent directory.
 *
 * On an exclusive create, the upper layer may know the object handle
 * already, so it MAY call with name == NULL. In this case, the caller
 * expects just to check the verifier.
 *
 * On a call with an existing object handle for an UNCHECKED create,
 * we can set the size to 0.
 *
 * At least the mode attribute must be set if createmode is not FSAL_NO_CREATE.
 * Some FSALs may still have to pass a mode on a create call for exclusive,
 * and even with FSAL_NO_CREATE, and empty set of attributes MUST be passed.
 *
 * If an open by name succeeds and did not result in Ganesha creating a file,
 * the caller will need to do a subsequent permission check to confirm the
 * open. This is because the permission attributes were not available
 * beforehand.
 *
 * The caller is expected to invoke fsal_release_attrs to release any
 * resources held by the set attributes. The FSAL layer MAY have added an
 * inherited ACL.
 *
 * The caller will set the request_mask in attrs_out to indicate the attributes
 * of interest. ATTR_ACL SHOULD NOT be requested and need not be provided. If
 * not all the requested attributes can be provided, this method MUST return
 * an error unless the ATTR_RDATTR_ERR bit was set in the request_mask.
 *
 * Since this method may instantiate a new fsal_obj_handle, it will be forced
 * to fetch at least some attributes in order to even know what the object
 * type is (as well as it's fileid and fsid). For this reason, the operation
 * as a whole can be expected to fail if the attributes were not able to be
 * fetched.
 *
 * The attributes will not be returned if this is an open by object as
 * opposed to an open by name.
 *
 * @note If the file was created, @a new_obj has been ref'd
 *
 * @param[in] obj_hdl               File to open or parent directory
 * @param[in,out] state             state_t to use for this operation
 * @param[in] openflags             Mode for open
 * @param[in] createmode            Mode for create
 * @param[in] name                  Name for file if being created or opened
 * @param[in] attrs_in              Attributes to set on created file
 * @param[in] verifier              Verifier to use for exclusive create
 * @param[in,out] new_obj           Newly created object
 * @param[in,out] attrs_out         Optional attributes for newly created object
 * @param[in,out] caller_perm_check The caller must do a permission check
 *
 * @return FSAL status.
 */
#endif

static fsal_status_t kvsfs_open2_by_handle(struct fsal_obj_handle *obj_hdl,
					   struct state_t *state,
					   fsal_openflags_t openflags,
					   struct attrlist *attrs_in,
					   struct attrlist *attrs_out,
					   bool *caller_perm_check)
{
	struct kvsfs_fsal_obj_handle *obj;
	int rc = 0;
	fsal_status_t status = fsalstat(ERR_FSAL_NO_ERROR, 0);
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_file_state *fd;
	struct stat stat;

	fd = &container_of(state, struct kvsfs_state_fd, state)->kvsfs_fd;
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	status = kvsfs_file_state_open(fd, openflags, obj, false);
	if (FSAL_IS_ERROR(status)) {
		goto out;
	}

	if (attrs_out != NULL) {
		rc = kvsns2_getattr(obj->fs_ctx, &cred,
				    &obj->handle->kvsfs_handle, &stat);
		if (rc < 0) {
			status = fsalstat(posix2fsal_error(-rc), -rc);
			goto out;
		}

		posix2fsal_attributes_all(&stat, attrs_out);
	}

	/* TODO:PORTING:
	 * Figure out when exactly we should set/unset this argument.
	 */
	if (caller_perm_check) {
		*caller_perm_check = true;
	}

out:
	return status;
}

static fsal_status_t kvsfs_open2_by_name(struct fsal_obj_handle *parent_obj_hdl,
					 const char *name,
					 struct fsal_obj_handle **new_obj_hdl,
					 struct state_t *state,
					 fsal_openflags_t openflags,
					 struct attrlist *attrs_in,
					 struct attrlist *attrs_out,
					 bool *caller_perm_check)
{
	fsal_status_t result;
	struct fsal_obj_handle *obj = NULL;

	assert(name);

	result = parent_obj_hdl->obj_ops->lookup(parent_obj_hdl, name, &obj,
						 NULL);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	assert(obj != NULL);

	result = kvsfs_open2_by_handle(obj, state, openflags, attrs_in,
				       attrs_out, caller_perm_check);
	if (FSAL_IS_ERROR(result)) {
		goto free_obj;
	}

	/* Give it to the caller */
	*new_obj_hdl = obj;
	obj = NULL;

free_obj:
	if (obj) {
		obj->obj_ops->release(obj);
	}
out:
	return result;
}

static fsal_status_t kvsfs_create2(struct fsal_obj_handle *parent_obj_hdl,
				   const char *name,
				   struct fsal_obj_handle **new_obj_hdl,
				   struct state_t *state,
				   fsal_openflags_t openflags,
				   enum fsal_create_mode createmode,
				   struct attrlist *attrs_in,
				   struct attrlist *attrs_out,
				   bool *caller_perm_check)
{
	struct fsal_obj_handle *obj_hdl = NULL;
	struct kvsfs_state_fd *state_fd =
		container_of(state, struct kvsfs_state_fd, state);
	struct kvsfs_file_state *fd = &state_fd->kvsfs_fd;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *parent_obj =
		container_of(parent_obj_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);
	kvsns_fs_ctx_t fs_ctx = parent_obj->fs_ctx;
	int rc;
	kvsns_ino_t object;
	fsal_status_t result;
	struct stat stat;
	mode_t unix_mode;

	assert(attrs_in);
	assert(createmode != FSAL_NO_CREATE);
	assert(name);

	/* TODO:PORTING
	 *   Check if `cred` should be extraced from attrs_in
	 *	instead of using op_ctx.
	 *   Check if we have enforce additional protections
	 *	(or loose them) for different createmode4 values.
	 */

	unix_mode = fsal2unix_mode(attrs_in->mode)
		& ~op_ctx->fsal_export->exp_ops.fs_umask(op_ctx->fsal_export);

	rc = kvsns2_creat(fs_ctx, &cred, &parent_obj->handle->kvsfs_handle,
			  (char *) name, unix_mode, &object);
	if (rc < 0) {
		result = fsalstat(posix2fsal_error(-rc), -rc);
		goto out;
	}

	result = kvsfs_open2_by_name(parent_obj_hdl,
				     name,
				     new_obj_hdl,
				     state,
				     openflags,
				     attrs_in,
				     attrs_out,
				     caller_perm_check);

out:
	return result;
}

static fsal_status_t kvsfs_open2(struct fsal_obj_handle *obj_hdl,
				 struct state_t *state,
				 fsal_openflags_t openflags,
				 enum fsal_create_mode createmode,
				 const char *name,
				 struct attrlist *attrs_in,
				 fsal_verifier_t verifier,
				 struct fsal_obj_handle **new_obj,
				 struct attrlist *attrs_out,
				 bool *caller_perm_check)
{
	struct attrlist verifier_attr = {0};
	fsal_status_t result;

	T_ENTER(">>> (obj=%p, st=%p, open=%d, create=%d,"
		" name=%s, attr_in=%p, verf=%d, new=%p,"
		" attr_out=%p, pcheck=%p)",
		obj_hdl, state, openflags, createmode,
		name, attrs_in, verifier[0], new_obj,
		attrs_out, caller_perm_check);

	T_TRACE("openflags: read=%d, write=%d, trunc=%d",
		(openflags & FSAL_O_READ),
		(openflags & FSAL_O_WRITE),
		(openflags & FSAL_O_TRUNC));

	assert(obj_hdl);

	/* TODO: Put a bunch of pre-conditions for cases like open_by_name,
	 * open_by_handle, create, exclusive create, etc.
	 * */

	if (state == NULL) {
		LogCrit(COMPONENT_FSAL,
			"KVSFS does not support NFSv3 clients.");
		result = fsalstat(ERR_FSAL_NOTSUPP, 0);
		goto out;
	}

	if (attrs_in != NULL) {
		LogAttrlist(COMPONENT_FSAL, NIV_FULL_DEBUG,
			    "attris_in ", attrs_in, false);
	}

	if (createmode == FSAL_NO_CREATE) {
		if (name == NULL) {
			result = kvsfs_open2_by_handle(obj_hdl, state,
						       openflags, attrs_in,
						       attrs_out,
						       caller_perm_check);
		} else {
			result = kvsfs_open2_by_name(obj_hdl, name, new_obj,
						     state, openflags,
						     attrs_in, attrs_out,
						     caller_perm_check);
		}
		goto out;
	} else {
		/* Now fixup attrs for verifier if exclusive create */
		if (createmode >= FSAL_EXCLUSIVE) {
			if (attrs_in) {
				set_common_verifier(attrs_in, verifier);
			} else {
				set_common_verifier(&verifier_attr, verifier);
			}
		}

		/* TODO:TRUNCATE:
		 * In this branch we can get a truncate call which we don't
		 * support yet. Use an ordinary open_by_handle instead.
		 */
		if ((openflags & FSAL_O_TRUNC) &&
		    (name == NULL) &&
		    createmode == FSAL_UNCHECKED) {
			assert(obj_hdl);
			result = kvsfs_open2_by_handle(obj_hdl,
					     state,
					     openflags,
					     attrs_in,
					     attrs_out,
					     caller_perm_check);
		} else {
			result = kvsfs_create2(obj_hdl, name, new_obj, state,
					       openflags, createmode, attrs_in,
					       attrs_out, caller_perm_check);
		}
	}

out:
	T_EXIT0(result.major);
	return result;
}

/******************************************************************************/
static fsal_status_t kvsfs_reopen2(struct fsal_obj_handle *obj_hdl,
				   struct state_t *state_hdl,
				   fsal_openflags_t openflags)
{
	struct kvsfs_fsal_obj_handle *obj;
	struct kvsfs_file_state *fd;
	fsal_status_t status;

	assert(obj_hdl);
	assert(state_hdl);

	T_ENTER(">>> (obj=%p, state=%p, openflags=%d)",
		obj_hdl, state_hdl, openflags);

	T_TRACE("openflags: read=%d, write=%d, trunc=%d",
		(openflags & FSAL_O_READ),
		(openflags & FSAL_O_WRITE),
		(openflags & FSAL_O_TRUNC));

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	fd = &container_of(state_hdl, struct kvsfs_state_fd, state)->kvsfs_fd;

	status = kvsfs_file_state_open(fd, openflags, obj, true);
	if (FSAL_IS_ERROR(status)) {
		goto out;
	}

out:
	T_EXIT0(status.major);
	return status;
}

/******************************************************************************/
/* FSAL.status2 */
static fsal_openflags_t kvsfs_status2(struct fsal_obj_handle *obj_hdl,
				      struct state_t *state)
{
	struct kvsfs_state_fd *state_fd;

	T_ENTER(">>> (obj=%p, state=%p)", obj_hdl, state);

	assert(state != NULL);

	state_fd = container_of(state, struct kvsfs_state_fd, state);

	T_EXIT0(0);
	return state_fd->kvsfs_fd.openflags;
}

/******************************************************************************/
/* FSAL.close2 */
static fsal_status_t kvsfs_close2(struct fsal_obj_handle *obj_hdl,
				  struct state_t *state)
{
	struct kvsfs_state_fd *state_fd;
	struct kvsfs_fsal_obj_handle *obj;

	T_ENTER(">>> (obj=%p, state=%p)", obj_hdl, state);

	assert(state != NULL);

	state_fd = container_of(state, struct kvsfs_state_fd, state);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	kvsfs_file_state_close(&state_fd->kvsfs_fd, obj);

	T_EXIT0(0);
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/******************************************************************************/
/* FSAL.close */
/* We don't support NFSv3 but NFS Ganesha still might want to close() a file
 * descriptior instead of a file state, for instance when finalizing
 * an export.
 */
static fsal_status_t kvsfs_close(struct fsal_obj_handle *obj_hdl)
{
	T_ENTER(">>> (obj=%p)", obj_hdl);
	/* Nothing to do because do not support NFSv3 OPEN */
	T_EXIT0(-1);
	return fsalstat(ERR_FSAL_NOT_OPENED, 0);
}

/******************************************************************************/
/* FSAL.read2 */
/* (copied from NFS Ganesha fsal_api)
 * @brief Read data from a file
 *
 * This function reads data from the given file. The FSAL must be able to
 * perform the read whether a state is presented or not. This function also
 * is expected to handle properly bypassing or not share reservations.  This is
 * an (optionally) asynchronous call.  When the I/O is complete, the done
 * callback is called with the results.
 *
 * @param[in]     obj_hdl	File on which to operate
 * @param[in]     bypass	If state doesn't indicate a share reservation,
 *				bypass any deny read
 * @param[in,out] done_cb	Callback to call when I/O is done
 * @param[in,out] read_arg	Info about read, passed back in callback
 * @param[in,out] caller_arg	Opaque arg from the caller for callback
 *
 * @return Nothing; results are in callback
 */
static void kvsfs_read2(struct fsal_obj_handle *obj_hdl,
			bool bypass,
			fsal_async_cb done_cb,
			struct fsal_io_arg *read_arg,
			void *caller_arg)
{
	struct kvsfs_fsal_obj_handle *obj;
	struct kvsfs_file_state *fd;
	uint64_t offset = read_arg->offset;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	fsal_status_t result;
	int i;
	ssize_t nb_read;
	void *buffer;
	size_t buffer_size;

	/* We support only NFSv4 clients. kvsfs_open2 won't allow
	 * an NFSv3 client to open file, therefore we won't end up here
	 * in this case. Ergo, the following check is an assert precondition
	 * but not an input parameter check.
	 */
	assert(read_arg->state);

	/* Since we don't support NFSv3, we cannot handle READ_PLUS.
	 * Note: do not confuse it with the NFSv4.2 READ_PLUS - it is not yet
	 * implemented in NFS Ganesha.
	 * */
	assert(read_arg->info == NULL);

	(void) bypass;

	T_ENTER(">>> (%p, %p, %llu)", obj_hdl, read_arg->state,
		(unsigned long long) read_arg->offset);

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	fd = &container_of(read_arg->state,
			   struct kvsfs_state_fd, state)->kvsfs_fd;

	for (i = 0; i < read_arg->iov_count; i++) {
		buffer = read_arg->iov[i].iov_base;
		buffer_size = read_arg->iov[i].iov_len;

		/* TODO:PERF: Scatter-gather IO.
		 * Add kvsns_readv and use it here.
		 */
		nb_read = kvsns2_read(obj->fs_ctx, &cred, &fd->kvsns_fd,
				     buffer, buffer_size, offset);
		if (nb_read < 0) {
			result = fsalstat(posix2fsal_error(-nb_read), -nb_read);
			goto out;
		}

		read_arg->io_amount += nb_read;
		offset += nb_read;
	}

	read_arg->end_of_file = (read_arg->io_amount == 0);

	result = fsalstat(ERR_FSAL_NO_ERROR, 0);
out:
	T_EXIT0(result.major);
	done_cb(obj_hdl, result, read_arg, caller_arg);
}

/******************************************************************************/
/* TODO:KVSNS: a dummy implementation of FSYNC call */
static int kvsns_fsync(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *ino)
{
	(void) ctx;
	(void) cred;
	(void) ino;
	return 0;
}

/******************************************************************************/
/* FSAL.write2 */
/* (copied from NFS Ganesha fsal_api)
 * @brief Write data to a file
 *
 * This function writes data to a file. The FSAL must be able to
 * perform the write whether a state is presented or not. This function also
 * is expected to handle properly bypassing or not share reservations. Even
 * with bypass == true, it will enforce a mandatory (NFSv4) deny_write if
 * an appropriate state is not passed).
 *
 * The FSAL is expected to enforce sync if necessary.
 *
 * This is an (optionally) asynchronous call.  When the I/O is complete, the @a
 * done_cb callback is called.
 *
 * @param[in]     obj_hdl       File on which to operate
 * @param[in]     bypass        If state doesn't indicate a share reservation,
 *                              bypass any non-mandatory deny write
 * @param[in,out] done_cb	Callback to call when I/O is done
 * @param[in,out] write_arg	Info about write, passed back in callback
 * @param[in,out] caller_arg	Opaque arg from the caller for callback
 */
static void kvsfs_write2(struct fsal_obj_handle *obj_hdl,
			 bool bypass,
			 fsal_async_cb done_cb,
			 struct fsal_io_arg *write_arg,
			 void *caller_arg)
{
	struct kvsfs_fsal_obj_handle *obj;
	struct kvsfs_file_state *fd;
	uint64_t offset = write_arg->offset;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	fsal_status_t result;
	int i;
	ssize_t nb_write;
	void *buffer;
	size_t buffer_size;
	int rc;

	/* We support only NFSv4 clients. kvsfs_open2 won't allow
	 * an NFSv3 client to open file, therefore we won't end up here
	 * in this case. Ergo, the following check is an assert precondition
	 * but not an input parameter check.
	 */
	assert(write_arg->state);
	/* Since we don't support NFSv3, we cannot handle WRITE_PLUS */
	assert(write_arg->info == NULL);

	/* TODO:SHARE_RESERVATION:BYPASS */
	/* TODO:SHARE_RESERVATION:STATE */
	(void) bypass;

	T_ENTER(">>> (%p, %p, %llu)", obj_hdl, write_arg->state,
		(unsigned long long) write_arg->offset);

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	fd = &container_of(write_arg->state,
			   struct kvsfs_state_fd, state)->kvsfs_fd;

	for (i = 0; i < write_arg->iov_count; i++) {
		buffer = write_arg->iov[i].iov_base;
		buffer_size = write_arg->iov[i].iov_len;

		/* TODO:PERF: Scatter-gather IO.
		 * Add kvsns_writev and use it here.
		 */
		nb_write = kvsns2_write(obj->fs_ctx, &cred, &fd->kvsns_fd,
				     buffer, buffer_size, offset);
		if (nb_write < 0) {
			result = fsalstat(posix2fsal_error(-nb_write), -nb_write);
			goto out;
		}

		/* TODO: Check nb_write against buffer_size here
		 * and try to write the rest of it.
		 * Otherwise, we may lose data.
		 */

		write_arg->io_amount += nb_write;
		offset += nb_write;
	}

	if (write_arg->fsal_stable) {
		nb_write = kvsns_fsync(obj->fs_ctx, &cred,
				       &obj->handle->kvsfs_handle);
		if (nb_write < 0) {
			write_arg->fsal_stable = false;
			result = fsalstat(posix2fsal_error(-nb_write), -nb_write);
			goto out;
		}
	}


	result = fsalstat(ERR_FSAL_NO_ERROR, 0);
out:
	T_EXIT0(result.major);
	done_cb(obj_hdl, result, write_arg, caller_arg);
}

/******************************************************************************/
/* FSAL.commit2 */
/* TODO:KVSNS does not support fsync() call, it always uses sync operations */
static fsal_status_t kvsfs_commit2(struct fsal_obj_handle *obj_hdl,
				   off_t offset, size_t len)
{
	(void) obj_hdl;
	(void) offset;
	(void) len;
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/******************************************************************************/
void kvsfs_handle_ops_init(struct fsal_obj_ops *ops)
{
	fsal_default_obj_ops_init(ops);

	// Namespace
	ops->release = release;
	ops->lookup = kvsfs_lookup;
	ops->readdir = kvsfs_readdir;
	ops->mkdir = kvsfs_mkdir;
	/* mknode is unsupported by KVSFS */

	ops->symlink = kvsfs_makesymlink;
	ops->readlink = kvsfs_readsymlink;

	ops->getattrs = kvsfs_getattrs;
	ops->setattr2 = kvsfs_setattrs;
	ops->link = kvsfs_linkfile;
	ops->rename = kvsfs_rename;
	ops->unlink = kvsfs_unlink;

	// Protocol
	ops->handle_to_wire = kvsfs_handle_digest;
	ops->handle_to_key = kvsfs_handle_to_key;

	// File state
	ops->open2 = kvsfs_open2;
	ops->status2 = kvsfs_status2;
	ops->close2 = kvsfs_close2;
	ops->close = kvsfs_close;
	ops->reopen2 = kvsfs_reopen2;

	// File IO
	ops->read2 = kvsfs_read2;
	ops->write2 = kvsfs_write2;
	ops->commit2 = kvsfs_commit2;

#if 0
	/* TODO:PORTING: replaced by lock_op2 */
	ops->lock_op2 = NULL /* kvsfs_lock_op */;
#endif


	/* TODO:PORTING: Disable xattrs support */
#if 0
	/* xattr related functions */
	ops->list_ext_attrs = kvsfs_list_ext_attrs;
	ops->getextattr_id_by_name = kvsfs_getextattr_id_by_name;
	ops->getextattr_value_by_name = kvsfs_getextattr_value_by_name;
	ops->getextattr_value_by_id = kvsfs_getextattr_value_by_id;
	ops->setextattr_value = kvsfs_setextattr_value;
	ops->setextattr_value_by_id = kvsfs_setextattr_value_by_id;
	ops->getextattr_attrs = kvsfs_getextattr_attrs;
	ops->remove_extattr_by_id = kvsfs_remove_extattr_by_id;
	ops->remove_extattr_by_name = kvsfs_remove_extattr_by_name;
#endif
}
