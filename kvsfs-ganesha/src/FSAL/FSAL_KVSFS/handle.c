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
	- TODO:EOS-1479:
		Special stateid handling.
	- TODO:EOS-914:
		Truncate op implementation.
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
#include <kvsns/kvsns_fh.h>

#include <../FSAL/Stackable_FSALs/FSAL_MDCACHE/mdcache_int.h>
#include <../FSAL/Stackable_FSALs/FSAL_MDCACHE/mdcache_hash.h>

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

struct kvsfs_fsal_obj_handle {
	/* Base Handle */
	struct fsal_obj_handle obj_handle;

	/* KVSNS Handle */
	kvsns_fh_t *handle;

	/* TODO:EOS-3288
	 * This field will removed and replaced
	 * by a call to kvsns_fh_t
	 */
	/* KVSNS Context */
	struct kvsfs_fsal_index_context *fs_ctx;

	/* Global state is disabled because we don't support NFv3. */
	/* struct kvsfs_file_state global_fd; */

	/* Share reservations */
	struct fsal_share share;
};

static inline kvsns_ino_t *
kvsfs_fh_to_ino(kvsns_fh_t *kvsfs_fh)
{
	return kvsns_fh_ino(kvsfs_fh);
}

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
/* TODO:
 * Function is deprecated after EOS-3288 is complitely done.
 */
static void construct_handle(struct fsal_export *export_base,
			     const kvsns_ino_t *ino,
			     const struct stat *stat,
			     struct fsal_obj_handle **obj)
{
	int rc;
	struct kvsfs_fsal_obj_handle *result;
	struct kvsfs_fsal_export *export =
	    container_of(op_ctx->fsal_export, struct kvsfs_fsal_export, export);

	result = gsh_calloc(1, sizeof(*result));

	/* A workaround to keep backward compatibilty:
	 * "construct_handle" was not designed to return an error code,
	 * so that in case of ENOMEM, we will just abort the program
	 * in the same as gsh_calloc() aborts execution if there is
	 * not enough memory.
	 */
	rc = kvsns_fh_from_ino(export->index_context, ino, stat, &result->handle);
	if (rc < 0) {
		LogCrit(COMPONENT_FSAL, "Failed to create FH, rc: %d", rc);
		abort();
	}

	result->fs_ctx = export->index_context;

	fsal_obj_handle_init(&result->obj_handle,
			     export_base,
			     posix2fsal_type(stat->st_mode));

	result->obj_handle.fsid.major = export->fs_id;
	result->obj_handle.fsid.minor = 0;
	result->obj_handle.fileid = stat->st_ino;

	result->obj_handle.obj_ops = &KVSFS.handle_ops;

	*obj = &result->obj_handle;

	T_TRACE("Constructed handle: %p, INO: %d, FSID: %d:%d, FILEID: %d",
		*obj, (int) *ino,
		(int) result->obj_handle.fsid.major,
		(int) result->obj_handle.fsid.minor,
		(int) result->obj_handle.fileid);
}

/** Creates a new KVSFS File Handle using a KVSNS File handle.
 * The function allocates and initializes a KVSFS FSAL FH using
 * an existing KVSNS Fh.
 * Note: It moves KVSNS FH into KVSFS FH and sets the pointer to
 * NULL -- it prevents consequent calls from directly accessing
 * KVSNS FH which is now a part of KVSFS FH.
 */
static void fsal_obj_handle_from_kvsns_fh(struct fsal_export *export_base,
					  kvsns_fh_t **pfh,
					  struct fsal_obj_handle **obj)
{
	struct kvsfs_fsal_obj_handle *result;
	kvsns_fh_t *fh = *pfh;
	struct kvsfs_fsal_export *export =
	    container_of(op_ctx->fsal_export, struct kvsfs_fsal_export, export);
	struct stat *stat = kvsns_fh_stat(fh);

	result = gsh_calloc(1, sizeof(*result));

	result->handle = fh;
	result->fs_ctx = export->index_context;

	fsal_obj_handle_init(&result->obj_handle,
			     export_base,
			     posix2fsal_type(stat->st_mode));

	/* TODO: We shoud not dynamically assign the FSID defined
	 * in the export config, and use stat->st_dev, i.e.:
	 *	result->obj_handle.fsid = posix2fsal_fsid(stat->st_dev);
	 * It is required to make sure that all file objects in all
	 * frontends have the correct fsid attribute.
	 * However, right now FSID is not propogated to stat->st_dev,
	 * so that we cannot use it.
	 */
	result->obj_handle.fsid.major = export->fs_id;
	result->obj_handle.fsid.minor = 0;
	result->obj_handle.fileid = stat->st_ino;

	result->obj_handle.obj_ops = &KVSFS.handle_ops;

	*obj = &result->obj_handle;
	*pfh = NULL; /* moved into the result */

	T_TRACE("Constructed KVSFS FH: %p, INO: %d, FSID: %d:%d, FILEID: %d",
		*obj, (int) *kvsns_fh_ino(fh),
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
/* Checks if a KVSFS File Handle is open.
 * NOTE: It works only with files passed by NFS Ganesha from the MDCACHE.
 * A file handle created by a kvsfs_lookup call won't show the correct state.
 */
static bool kvsfs_fh_is_open(struct kvsfs_fsal_obj_handle *obj)
{
	static const struct fsal_share empty_share = {0};
	return (memcmp(&empty_share, &obj->share, sizeof(empty_share)) != 0);
}

/******************************************************************************/
/* FSAL.lookup */
static fsal_status_t kvsfs_lookup(struct fsal_obj_handle *parent_hdl,
				  const char *name,
				  struct fsal_obj_handle **handle,
				  struct attrlist *attrs_out)
{
	struct kvsfs_fsal_obj_handle *parent;
	int rc = 0;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_fh_t *object = NULL;
	struct kvsfs_fsal_export *export =
	    container_of(op_ctx->fsal_export, struct kvsfs_fsal_export, export);

	T_ENTER(">>> (%p, %s)", parent_hdl, name);

	assert(name);
	assert(strlen(name) > 0);

	if (!fsal_obj_handle_is(parent_hdl, DIRECTORY)) {
		LogCrit(COMPONENT_FSAL,
			"Parent handle (%p) is not a directory.", parent_hdl);
		rc = -ENOTDIR;
		goto out;
	}

	parent = container_of(parent_hdl, struct kvsfs_fsal_obj_handle,
			     obj_handle);

	rc = kvsns_fh_lookup(&cred, parent->handle, name, &object);

	if (rc < 0) {
		goto out;
	}

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(kvsns_fh_stat(object), attrs_out);
	}

	fsal_obj_handle_from_kvsns_fh(op_ctx->fsal_export, &object, handle);

 out:
	if (object) {
		kvsns_fh_destroy(object);
	}
	T_EXIT0(-rc);
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/******************************************************************************/
/* FSAL.looup_path - Find Inode of Export Root */
fsal_status_t kvsfs_lookup_path(struct fsal_export *exp_hdl,
			       const char *path,
			       struct fsal_obj_handle **handle,
			       struct attrlist *attrs_out)
{
	int rc = 0;
	kvsns_fh_t *object = NULL;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_export *myexport;

	T_ENTER(" >> (%p, %s)", exp_hdl, path);
	assert(exp_hdl);

	myexport = container_of(op_ctx->fsal_export,
				struct kvsfs_fsal_export, export);

	rc = kvsns_fh_getroot(myexport->index_context, &cred, &object);
	if (rc != 0) {
		goto out;
	}

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(kvsns_fh_stat(object), attrs_out);
	}

	fsal_obj_handle_from_kvsns_fh(op_ctx->fsal_export, &object, handle);

out:
	if (object) {
		kvsns_fh_destroy(object);
	}
	T_EXIT0(-rc);
	return fsalstat(posix2fsal_error(-rc), -rc);
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
			     kvsfs_fh_to_ino(myself->handle),
			     (char *) name, unix_mode, &object);
	if (retval < 0) {
		goto out;
	}

	retval = kvsns_getattr(myself->fs_ctx, &cred, &object, &stat);
	if (retval < 0) {
		/* XXX:
		 * We have created a directory but cannot get its attributes
		 * What should we do here? remove it?
		 *
		 * TODO:PERF:
		 * kvsns_mkdir must return the stats of the created
		 * directory so that we can avoid this kvsns_getattr call.
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
			       kvsfs_fh_to_ino(myself->handle),
			       (char *)name, (char *)link_path, &object);
	if (retval < 0) {
		status = fsalstat(posix2fsal_error(-retval), -retval);
		goto out;
	}

	retval = kvsns_getattr(myself->fs_ctx, &cred, &object, &stat);
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
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	if (!fsal_obj_handle_is(obj_hdl, SYMBOLIC_LINK)) {
		retval = -EINVAL; /* See RFC7530, 16.25.5 */
		goto out;
	}

	myself = container_of(obj_hdl, struct kvsfs_fsal_obj_handle,
			      obj_handle);

	/* TODO:PERF:
	 * We are allocating a 4K buffer here. We should not allocate it every
	 * time.
	 * See additional details here: EOS-259.
	 */
	link_content->len = fsal_default_linksize;
	link_content->addr = gsh_malloc(link_content->len);

	retval = kvsns_readlink(myself->fs_ctx, &cred,
				kvsfs_fh_to_ino(myself->handle),
				link_content->addr, &link_content->len);

	if (retval < 0) {
		gsh_free(link_content->addr);
		link_content->addr = NULL;
		link_content->len = 0;
		goto out;
	}

	/* NFS Ganesha requires the content buffer to be a NULL-terminated
	 * string. Let's try to null-terminate it.
	 * If the string len is 4K then we will just truncate it in the same
	 * way as readlink(2) truncates the output buffer.
	 */
	if (link_content->len == fsal_default_linksize) {
		/* truncate and append null */
		((char *) link_content->addr)[link_content->len - 1] = '\0';
	} else {
		/* append null */
		((char *) link_content->addr)[link_content->len] = '\0';
		link_content->len++;
	}

 out:
	return fsalstat(posix2fsal_error(-retval), -retval);
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
			    kvsfs_fh_to_ino(myself->handle),
			    kvsfs_fh_to_ino(destdir->handle),
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
		cb_ctx->where++;
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

		retval = kvsns_getattr(cb_ctx->parent->fs_ctx, &cred, &object,
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
		/* If whence is NULL, it means we should start from the very
		 * beginning (dentry[0]).
		 * Othwerwise, it indicates the index of the dentry
		 * (dentry[N]) followed by the dentry (dentry[N+1]) we should
		 * pass to the callback.
		 */
		.whence = whence ? ((*whence) + 1) : KVSFS_FIRST_COOKIE,
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

	T_ENTER("parent=%d", (int) *kvsfs_fh_to_ino(obj->handle));

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
	rc = kvsns_readdir(fs_ctx, &cred, kvsfs_fh_to_ino(obj->handle),
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
static fsal_status_t kvsfs_unlink_reg(struct fsal_obj_handle *dir_hdl,
				      struct fsal_obj_handle *obj_hdl,
				      const char *name);

/* FIXME: MDCACHE does not expose find_keyed anymore.
 * In order to support delete-on-close for RENAME op, we need to figure out
 * another way to get a cached file handle for the file that
 * needs to be removed prior rename.
 */
#if 0
/* Finds a file object cached in the inode cache.
 * The inode cache (MDCACHE) must keep an in-memory file handle structure
 * if the corresponding file is open.
 * One of the use cases is a rename() call where a client
 * is trying to overwrite an existing regular file with another regular file.
 * The kvsfs_rename needs the information ('share' field from the FH) about
 * existing open states in order to determine if the file needs to be removed
 * or not.
 * NOTE: This function returns only the cached FH but it does not return
 * an uncached FH (created with kvsfs_lookup).
 */
fsal_status_t kvsfs_find_in_mdcache(struct fsal_obj_handle *obj,
				    struct fsal_obj_handle **obj_cached)
{
	struct gsh_buffdesc fh_desc;
	mdcache_entry_t *mdc_obj_hdl;
	mdcache_key_t key;
	fsal_status_t result;

	/* Get a key to be used for searching in the MDCACHE. */
	obj->obj_ops->handle_to_key(obj, &fh_desc);

	(void) cih_hash_key(&key, op_ctx->fsal_export->fsal, &fh_desc,
			    CIH_HASH_KEY_PROTOTYPE);

	/* Try to find it in the cache. */
	result = mdcache_find_keyed(&key, &mdc_obj_hdl);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	/* Get the KVSFS file handle */
	*obj_cached = mdc_obj_hdl->sub_handle;

out:
	return result;
}
#else
fsal_status_t kvsfs_find_in_mdcache(struct fsal_obj_handle *obj,
				    struct fsal_obj_handle **obj_cached)
{
	(void) obj;
	(void) obj_cached;
	return fsalstat(posix2fsal_error(ENOENT), ENOENT);
}
#endif

/* FSAL.rename */
/** Rename (re-link) an object in a filesystem.
 *  @param[in] obj_hdl An existing object in the FS to be renamed.
 *  @param[in] olddir_hdl A parent dir where the object is linked under the
 *  name `old_name`.
 *  @param[in] old_name The current name of the object.
 *  @param[in] newdir_hdl A dir where the object will be linked under the name
 *  `new_name`.
 *  @param[in] new_name A name of the object.
 *  @return @see kvsns_rename error codes.
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
	struct kvsfs_fsal_obj_handle *newobj;
	struct fsal_obj_handle *kvsfs_obj_hdl = NULL;
	struct fsal_obj_handle *newobj_hdl = NULL;
	struct kvsns_rename_flags flags = KVSNS_RENAME_FLAGS_INIT;
	fsal_status_t result;
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
		goto out;
	}
	if (obj->fs_ctx != newdir->fs_ctx) {
		LogFatal(COMPONENT_FSAL,
			 "obj fs_ctx does not match newdir fs_ctx "
			 "'%p' != '%p'", obj_hdl, newdir_hdl);
		rc = -EXDEV;
		goto out;
	}
#endif

	result = newdir_hdl->obj_ops->lookup(newdir_hdl, new_name, &kvsfs_obj_hdl, NULL);
	if (result.major == ERR_FSAL_NOENT) {
		/* Destination object does not exist -> go on and rename. */
		newobj_hdl = NULL;
		goto rename;
	}
	if (FSAL_IS_ERROR(result)) {
		LogFatal(COMPONENT_FSAL,
			 "Failed to lookup destination object: %p/%s",
			 newdir_hdl, new_name);
		goto out;
	}

	/* It is a file handle which will be removed from the file system.
	 * By default use the FH found by kvsfs_lookup() call.
	 * */
	newobj_hdl = kvsfs_obj_hdl;

	if (fsal_obj_handle_is(newobj_hdl, REGULAR_FILE)) {
		/* Use an FH from the mdcache if it exists in the cache */
		(void) kvsfs_find_in_mdcache(kvsfs_obj_hdl, &newobj_hdl);
	}

	newobj = container_of(newobj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	flags.is_dst_open = kvsfs_fh_is_open(newobj);

rename:
	rc = kvsns_rename(obj->fs_ctx, &cred,
			  kvsfs_fh_to_ino(olddir->handle), (char *) old_name,
			  kvsfs_fh_to_ino(obj->handle),
			  kvsfs_fh_to_ino(newdir->handle), (char *) new_name,
			  newobj_hdl ? kvsfs_fh_to_ino(newobj->handle) : NULL,
			  &flags);

	result = fsalstat(posix2fsal_error(-rc), -rc);

out:
	if (kvsfs_obj_hdl) {
		kvsfs_obj_hdl->obj_ops->release(kvsfs_obj_hdl);
	}
	return result;

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

	if (attrs_out == NULL) {
		retval = 0;
		goto out;
	}

	myself =
		container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	retval = kvsns_getattr(myself->fs_ctx, &cred,
				kvsfs_fh_to_ino(myself->handle), &stat);
	if (retval < 0) {
		goto out;
	}

	posix2fsal_attributes_all(&stat, attrs_out);

out:
	fsal_error = posix2fsal_error(-retval);
	T_EXIT0(-retval);
	return fsalstat(fsal_error, -retval);
}

/******************************************************************************/
/* FSAL.getattr */

/* a jury-rigged convertor from NFS attrs into POSIX stats */
static fsal_status_t kvsfs_setattr_attrlist2stat(struct attrlist *attrs,
						 struct stat *stats_out,
						 int *flags_out)
{
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
	struct stat stats = { 0};
	int flags = 0;

	/* apply umask, if mode attribute is to be changed */
	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_MODE)) {
		attrs->mode &= ~op_ctx->fsal_export->exp_ops.
				fs_umask(op_ctx->fsal_export);
	}

	if (FSAL_TEST_MASK(attrs->valid_mask, ATTR_SIZE)) {
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

/* INTERNAL */
static fsal_status_t kvsfs_ftruncate(struct fsal_obj_handle *obj,
				     struct state_t *state, bool bypass,
				     struct stat *new_stat, int new_stat_flags);

/* FSAL.setattrs2 */
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

	T_ENTER(">>> (obj=%p, bypass=%d, state=%p, attrs=%p)", obj_hdl,
		bypass, state, attrs);

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	result = kvsfs_setattr_attrlist2stat(attrs, &stats, &flags);
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
		result = kvsfs_ftruncate(obj_hdl, state, bypass, &stats, flags);
	} else {
		/* If the size does not need to be change, then
		 * we can simply update the stats associated with the inode
		 */
		rc = kvsns_setattr(obj->fs_ctx, &cred,
				    kvsfs_fh_to_ino(obj->handle),
				    &stats, flags);
		result = fsalstat(posix2fsal_error(-rc), -rc);
	}

out:
	T_EXIT0(result.major);
	return result;
}

/******************************************************************************/
/* INTERNAL */
static fsal_status_t kvsfs_unlink_reg(struct fsal_obj_handle *dir_hdl,
				      struct fsal_obj_handle *obj_hdl,
				      const char *name)
{
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *parent;
	struct kvsfs_fsal_obj_handle *obj;
	int rc;

	parent = container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	rc = kvsns_detach(parent->fs_ctx, &cred, kvsfs_fh_to_ino(parent->handle),
			  kvsfs_fh_to_ino(obj->handle), name);
	if (rc != 0) {
		goto out;
	}

	/* FIXME: We might try to use rdlock here instead of wrlock because
	 * we just need to avoid writing in obj->share. But we don't have have
	 * atomic operations in KVSNS yet, so that let's just serialize access
	 * to destroy_file by enforcing wrlock. */
	PTHREAD_RWLOCK_wrlock(&obj->obj_handle.obj_lock);
	if (kvsfs_fh_is_open(obj)) {
		/* Postpone removal until close */
		rc = 0;
	} else {
		/* No share states detected  -> Delete file object */
		rc = kvsns_destroy_orphaned_file(parent->fs_ctx,
						 kvsfs_fh_to_ino(obj->handle));
	}
	PTHREAD_RWLOCK_unlock(&obj->obj_handle.obj_lock);

out:
	if (rc != 0 ) {
		LogCrit(COMPONENT_FSAL, "Failed to unlink reg file"
			" %llu/%llu '%s'",
			(unsigned long long) *kvsfs_fh_to_ino(parent->handle),
			(unsigned long long) *kvsfs_fh_to_ino(obj->handle),
			name);
	}
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/* INTERNAL */
static fsal_status_t kvsfs_rmsymlink(struct fsal_obj_handle *dir_hdl,
				     struct fsal_obj_handle *obj_hdl,
				     const char *name)
{
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *parent;
	struct kvsfs_fsal_obj_handle *obj;
	int rc;

	parent = container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	rc = kvsns_unlink(parent->fs_ctx, &cred,
			  kvsfs_fh_to_ino(parent->handle),
			  kvsfs_fh_to_ino(obj->handle),
			  (char *) name);
	if (rc != 0) {
		LogCrit(COMPONENT_FSAL, "Failed to rmdir name=%s, rc=%d",
			name, rc);
		goto out;
	}

out:
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/* INTERNAL */
static fsal_status_t kvsfs_rmdir(struct fsal_obj_handle *dir_hdl,
				 struct fsal_obj_handle *obj_hdl,
				 const char *name)
{
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *parent;
	int rc;

	parent = container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	/* TODO:PERF:
	 * Look up has already been done by fsal_remove(),
	 * so that we can freely pass `obj_hdl` into rmdir()
	 * in order to avoid the extra lookup() call inside
	 * rmdir().
	 */

	rc = kvsns_rmdir(parent->fs_ctx, &cred,
			  kvsfs_fh_to_ino(parent->handle),
			  (char *) name);
	if (rc != 0) {
		LogCrit(COMPONENT_FSAL, "Failed to rmdir name=%s, rc=%d",
			name, rc);
		goto out;
	}

out:
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/* FSAL.unlink - Unlink a file or a directory. */
static fsal_status_t kvsfs_remove(struct fsal_obj_handle *dir_hdl,
				  struct fsal_obj_handle *obj_hdl,
				  const char *name)
{
	struct kvsfs_fsal_obj_handle *parent;
	fsal_status_t result;

	if (fsal_obj_handle_is(obj_hdl, DIRECTORY)) {
		result = kvsfs_rmdir(dir_hdl, obj_hdl, name);
	} else if (fsal_obj_handle_is(obj_hdl, REGULAR_FILE)) {
		result = kvsfs_unlink_reg(dir_hdl, obj_hdl, name);
	} else if (fsal_obj_handle_is(obj_hdl, SYMBOLIC_LINK)) {
		result = kvsfs_rmsymlink(dir_hdl, obj_hdl, name);
	}

	return result;
}

/******************************************************************************/
/* FSAL.handle_to_wire */

/* Converts in-memory object into opaque byte array.
 * The byte array is used by the clients as an id of files or directories
 * (or other file objects).
 */
static fsal_status_t kvsfs_handle_digest(const struct fsal_obj_handle *obj_hdl,
					fsal_digesttype_t output_type,
					struct gsh_buffdesc *fh_desc)
{
	const struct kvsfs_fsal_obj_handle *myself;
	const size_t fh_size = kvsns_fh_serialized_size();

	assert(fh_desc);
	assert(obj_hdl);

	if (output_type != FSAL_DIGEST_NFSV4) {
		LogMajor(COMPONENT_FSAL,
			 "Only NFSv4 File handles are supported."
			 "Unsupported FH type: %d", output_type);
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}

	myself = container_of(obj_hdl, const struct kvsfs_fsal_obj_handle,
			 obj_handle);

	if (fh_size > fh_desc->len) {
		LogMajor(COMPONENT_FSAL,
			 "Space too small for handle (need %d, have %d).",
			 (int) fh_size, (int) fh_desc->len);
		return fsalstat(ERR_FSAL_TOOSMALL, ENOBUFS);
	}

	fh_desc->len = kvsns_fh_ser_with_fsid(myself->handle, obj_hdl->fsid.major,
					      fh_desc->addr,
					      fh_desc->len);

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
	const size_t fh_size = kvsns_fh_serialized_size();

	assert(fh_desc);

	if (in_type != FSAL_DIGEST_NFSV4) {
		LogMajor(COMPONENT_FSAL,
			 "Only NFSv4 File handles are supported."
			 "Unsupported FH type: %d", in_type);
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}


	if (fh_size < fh_desc->len) {
		LogMajor(COMPONENT_FSAL,
			 "Size mismatch for handle.  should be %d, got %d",
			 (int)  fh_size,
			 (int) fh_desc->len);
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
	kvsns_fh_key(myself->handle, &fh_desc->addr, &fh_desc->len);
}

/******************************************************************************/
/* FSAL_EXPORT.create_handle */

/* Create handle for export */
fsal_status_t kvsfs_create_handle(struct fsal_export *exp_hdl,
				  struct gsh_buffdesc *hdl_desc,
				  struct fsal_obj_handle **handle,
				  struct attrlist *attrs_out)
{
	int rc = 0;
	kvsns_fh_t *fh = NULL;
	struct kvsfs_fsal_export *myexport;
	/* FIXME: It is unclear yet if this function is a subject to access
	 * checks */
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	assert(exp_hdl);
	assert(hdl_desc);
	assert(hdl_desc->addr);
	assert(handle);

	myexport = container_of(op_ctx->fsal_export,
				struct kvsfs_fsal_export, export);

	rc = kvsns_fh_deserialize(myexport->index_context, &cred,
				  hdl_desc->addr,
				  hdl_desc->len, &fh);
	if (rc < 0) {
		goto out;
	}

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(kvsns_fh_stat(fh), attrs_out);
	}

	fsal_obj_handle_from_kvsns_fh(exp_hdl, &fh, handle);

out:
	if (fh) {
		kvsns_fh_destroy(fh);
	}

	return fsalstat(posix2fsal_error(-rc), -rc);
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
/* A default invalid value for an inode number. */
#define KVSNS_INVALID_INO_FOR_FD 0
static inline
bool kvsfs_file_state_invariant_closed(const struct kvsfs_file_state *state)
{
	return (state->openflags == FSAL_O_CLOSED) &&
		(state->kvsns_fd.ino == KVSNS_INVALID_INO_FOR_FD);
}

static inline
bool kvsfs_file_state_invariant_open(const struct kvsfs_file_state *state)
{
	return (state->openflags != FSAL_O_CLOSED) &&
		(state->kvsns_fd.ino != KVSNS_INVALID_INO_FOR_FD);
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
	state->kvsns_fd.ino = *kvsfs_fh_to_ino(obj->handle);

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
	state->kvsns_fd.ino = KVSNS_INVALID_INO_FOR_FD;

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
/** Find a readable/writeable FD using a file state (including locking state)
 * as a base.
 * Sometimes an NFS Client may try to use a stateid associated with a state lock
 * to read/write/setattr(truncate) a file. Such a stateid cannot be used
 * directly in read/write calls. However, it is possible to use this state
 * to identify the correponding open state which, in turn, can be used in
 * IO operations.
 * NFS Ganesha has similiar function ::fsal_find_fd which does the same
 * thing but it has 13 arguments and handles all possible scenarious like
 * NFSv3, share reservation checks, NFSv4 open, NFSv4 locks, NLM locks
 * and so on. On contrary, kvsfs_find_fd handles only NFSv4 open and locked
 * states. In future, we may have to add handling of bypass mode
 * (special stateids, see EOS-1479).
 * @param fsal_state A file state passed down from NFS Ganesha for IO callback.
 * @param bypass Bypass share reservations flag.
 * @param[out] fd Pointer to an FD to be filled.
 * @return So far always returns OK.
 */
static fsal_status_t kvsfs_find_fd(struct state_t *fsal_state,
				   bool bypass,
				   fsal_openflags_t openflags,
				   struct kvsfs_file_state **fd)
{
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	struct kvsfs_state_fd *kvsfs_state;

	T_ENTER(">>> (state=%p, bypass=%d, openflags=%d, fd=%p)",
		fsal_state, (int) bypass, (int) openflags, fd);

	assert(fd != NULL);

	assert(fsal_state->state_type == STATE_TYPE_LOCK ||
	       fsal_state->state_type == STATE_TYPE_SHARE ||
	       fsal_state->state_type == STATE_TYPE_DELEG);

	/* TODO:EOS-1479: We don't suppport special state ids yet. */
	assert(bypass == false);
	assert(fsal_state != NULL);

	/* We don't have an open file for locking states,
	 * therefore let's just reuse the associated open state.
	 */
	if (fsal_state->state_type == STATE_TYPE_LOCK) {
		fsal_state = fsal_state->state_data.lock.openstate;
		assert(fsal_state != NULL);
	}

	kvsfs_state = container_of(fsal_state, struct kvsfs_state_fd, state);

	if (open_correct(kvsfs_state->kvsfs_fd.openflags, openflags)) {
		*fd = &kvsfs_state->kvsfs_fd;
		goto out;
	}

	/* if there is no suitable FD for this fsal_state then it is likely
	 * has been caused by a state type which we cannot handle right now.
	 * Let's just print some logs and then fail right here.
	 */
	LogCrit(COMPONENT_FSAL, "Unsupported state type: %d",
		(int) fsal_state->state_type);
	assert(0); /* Unreachable */

out:
	T_EXIT0(result.major);
	return result;
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
	super->kvsfs_fd.kvsns_fd.ino = KVSNS_INVALID_INO_FOR_FD;

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
/* FSAL.lease_op2 */
static fsal_status_t kvsfs_lease_op2(struct fsal_obj_handle *obj_hdl,
				     struct state_t *state_hdl,
				     void *owner,
				     fsal_deleg_t deleg)
{
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	struct kvsfs_state_fd *state;
	struct kvsfs_fsal_obj_handle *obj;

	T_ENTER(">>> (obj_hdl=%p, state=%p, owner=%p, deleg=%d)",
		obj_hdl, state_hdl, owner, (int) deleg);

	assert(obj_hdl);
	assert(state_hdl);
	assert(state_hdl->state_type == STATE_TYPE_DELEG);

	state = container_of(state_hdl, struct kvsfs_state_fd, state);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	/* We are not working in clustered environment yet,
	 * so that let's leave conflicts detection to NFS Ganesha.
	 */

	switch (deleg) {
	case FSAL_DELEG_NONE:
		T_TRACE("%s", "Releasing delegation");
		assert(kvsfs_file_state_invariant_open(&state->kvsfs_fd));
		result = kvsfs_file_state_close(&state->kvsfs_fd, obj);
		break;

	/* For Read and Write delegation we can simply "open" the file for
	 * RD or WR mode and then in find_fd extract this FD from the
	 * kvsfs state in the same way as it works with "Share" states,
	 * thus avoiding the need to open anything inside READ/WRITE calls
	 * when the client would like to read out or to flush data back
	 * to the server.
	 * FIXME: We don't know yet if it is possible to get a reopen-like
	 * sequence of calls from NFS Ganesha (from the same client), so that
	 * we assume that it will never happen and passing reopen=false
	 * for the READ and WRITE open calls.
	 */
	case FSAL_DELEG_RD:
		T_TRACE("%s", "Read delegation");
		assert(kvsfs_file_state_invariant_closed(&state->kvsfs_fd));
		result = kvsfs_file_state_open(&state->kvsfs_fd, FSAL_O_READ,
					       obj, false);
		break;
	case FSAL_DELEG_WR:
		T_TRACE("%s", "Write delegation");
		assert(kvsfs_file_state_invariant_closed(&state->kvsfs_fd));
		result = kvsfs_file_state_open(&state->kvsfs_fd, FSAL_O_WRITE,
					       obj, false);
		break;
	}

	T_EXIT0(result.major);
	return result;
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
		rc = kvsns_getattr(obj->fs_ctx, &cred,
				    kvsfs_fh_to_ino(obj->handle), &stat);
		if (rc < 0) {
			status = fsalstat(posix2fsal_error(-rc), -rc);
			goto out;
		}

		posix2fsal_attributes_all(&stat, attrs_out);
	}

	/* kvsfs_file_state_open does not call test_access and
	 * kvsns_getattrs also does not check it. Therefore, let the caller
	 * check access.
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

	result = kvsfs_open2_by_handle(obj, state, openflags,
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

/** Implementation of OPEN4+UNCHECKED4 case when the file exists. */
static fsal_status_t kvsfs_open_unchecked(struct fsal_obj_handle *obj_hdl,
					  struct state_t *state,
					  fsal_openflags_t openflags,
					  struct attrlist *attrs_in,
					  struct attrlist *attrs_out,
					  bool *caller_perm_check)
{
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	bool close_on_exit = false;
	struct stat stat = {0};
	int flags = 0;

	T_ENTER(">>> (obj=%p, state=%p, attrs_in=%p, attrs_out=%p)",
		obj_hdl, state, attrs_in, attrs_out);

	assert(obj_hdl);
	assert(attrs_in);
	assert(state);
	assert(obj_hdl->type == REGULAR_FILE);

	result = kvsfs_open2_by_handle(obj_hdl, state, openflags, attrs_out,
				       caller_perm_check);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	close_on_exit = true;

	/* TODO: When all OPEN4 cases are implemented,
	 * this O_TRUNC processing might be integrated into open_by_handle.
	 */
	if ((openflags & FSAL_O_TRUNC) != 0) {
		flags |= STAT_SIZE_SET;
		stat.st_size = 0;
		result = kvsfs_ftruncate(obj_hdl, state, false, &stat, flags);
		if (FSAL_IS_ERROR(result)) {
			goto out;
		}
		/* We don't need to change attrs_out->filesize here
		 * because its is unset by Ganesha already.
		 */
	}

	close_on_exit = false;
out:
	if (close_on_exit) {
		(void) obj_hdl->obj_ops->close2(obj_hdl, state);
	}
	T_EXIT0(result.major);
	return result;
}



/** Implementation of OPEN4+UNCHECKED4 case when the file does not exist. */
static fsal_status_t
kvsfs_create_unchecked(struct fsal_obj_handle *parent_obj_hdl, const char *name,
		       struct state_t *state, struct fsal_obj_handle **pnew_obj,
		       fsal_openflags_t openflags, struct attrlist *attrs_in,
		       struct attrlist *attrs_out, bool *caller_perm_check)
{
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	int rc;
	struct kvsfs_fsal_obj_handle *parent_obj;
	struct fsal_obj_handle *obj_hdl = NULL;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_ino_t object;
	struct stat stat_in;
	struct stat stat_out;
	int flags;

	T_ENTER(">>> (parent=%p, state=%p, name=%s, attrs_in=%p, attrs_out=%p)",
		parent_obj_hdl, state, name, attrs_in, attrs_out);

	/* Ganesha sets ATTR_MODE even if the client didn't set it */
	assert(FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_MODE));
	assert(name != NULL);
	assert(state && parent_obj_hdl);

	parent_obj = container_of(parent_obj_hdl,
				  struct kvsfs_fsal_obj_handle, obj_handle);

	result = kvsfs_setattr_attrlist2stat(attrs_in, &stat_in, &flags);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	rc = kvsns_creat_ex(parent_obj->fs_ctx, &cred,
			    kvsfs_fh_to_ino(parent_obj->handle),
			    (char *) name,
			    stat_in.st_mode,
			    &stat_in, flags,
			    &object,
			    &stat_out);
	if (rc < 0) {
		result = fsalstat(posix2fsal_error(-rc), -rc);
		goto out;
	}

	construct_handle(op_ctx->fsal_export, &object, &stat_out, &obj_hdl);

	result = kvsfs_open2_by_handle(obj_hdl, state, openflags, NULL, NULL);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat_out, attrs_out);
	}

	*pnew_obj = obj_hdl;
	obj_hdl = NULL;

	/* We have already checked permissions in kvsns_create */
	if (caller_perm_check) {
		*caller_perm_check = false;
	}

	T_TRACE("Created: %p", *pnew_obj);

out:
	if (obj_hdl != NULL) {
		obj_hdl->obj_ops->release(obj_hdl);
	}
	T_EXIT0(result.major);
	return result;
}

/** Implementation of OPEN4+EXCLUSIVE4 case when the file does not exist.
 * At first NFS Ganesha checks if the file exists. If it doesn't then ganesha
 * calls the fsal open2 callback. If it exists then ganesha checks the verifier
 * in order to determine if it is a retransmission.
 */
static fsal_status_t
kvsfs_create_exclusive40(struct fsal_obj_handle *parent_obj_hdl, const char *name,
			 struct state_t *state, struct fsal_obj_handle **pnew_obj,
			 fsal_openflags_t openflags, struct attrlist *attrs_in,
			 struct attrlist *attrs_out, bool *caller_perm_check,
			 fsal_verifier_t verifier)
{
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	int rc;
	struct kvsfs_fsal_obj_handle *parent_obj;
	struct fsal_obj_handle *obj_hdl = NULL;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	kvsns_ino_t object;
	struct stat stat_in;
	struct stat stat_out;
	int flags;

	/* attrs_in is not empty but only fattr.mode is set. */
	assert(attrs_in);
	/* but only mode is set by NFS Ganesha */
	assert(attrs_in->valid_mask == ATTR_MODE);

	assert(name);

	/* Use ATIME and MTIME attrs as verifiers. */
	set_common_verifier(attrs_in, verifier);

	assert(FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_ATIME));
	assert(FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_MTIME));
	assert(FSAL_TEST_MASK(attrs_in->valid_mask, ATTR_MODE));

	/* create operations do not support the truncate flag */
	assert((openflags & FSAL_O_TRUNC) == 0);

	result = kvsfs_setattr_attrlist2stat(attrs_in, &stat_in, &flags);
	if (FSAL_IS_ERROR(result)) {
		/* this function cannot fail if we are setting only
		 * atime, mtime and mode.
		 */
		assert(0);
		goto out;
	}

	parent_obj = container_of(parent_obj_hdl,
				  struct kvsfs_fsal_obj_handle, obj_handle);

	rc = kvsns_creat_ex(parent_obj->fs_ctx, &cred,
			    kvsfs_fh_to_ino(parent_obj->handle),
			    (char *) name,
			    stat_in.st_mode,
			    &stat_in, flags,
			    &object,
			    &stat_out);
	if (rc < 0) {
		result = fsalstat(posix2fsal_error(-rc), -rc);
		goto out;
	}

	construct_handle(op_ctx->fsal_export, &object, &stat_out, &obj_hdl);

	result = kvsfs_open2_by_handle(obj_hdl, state, openflags, NULL, NULL);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	*pnew_obj = obj_hdl;
	obj_hdl = NULL;

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&stat_out, attrs_out);
	}

	/* We have already checked permissions in kvsns_creat_ex */
	if (caller_perm_check) {
		*caller_perm_check = false;
	}

	T_TRACE("Created: %p", *pnew_obj);

out:
	if (obj_hdl != NULL) {
		obj_hdl->obj_ops->release(obj_hdl);
	}
	T_EXIT0(result.major);
	return result;
}

/* Open a file which has been created with O_EXCL but was not opened. */
static fsal_status_t
kvsfs_open_exclusive40(struct fsal_obj_handle *obj_hdl,
		       struct state_t *state,
		       fsal_openflags_t openflags,
		       struct attrlist *attrs_in,
		       struct attrlist *attrs_out,
		       bool *caller_perm_check)
{
	fsal_status_t result;

	T_ENTER(">>> (obj=%p, st=%p, open=%d, pcheck=%p)",
		obj_hdl, state, openflags, caller_perm_check);

	assert(obj_hdl);

	/* attrs_in is set */
	assert(attrs_in);
	/* but only fattr.mode is set by NFS Ganesha. And in an open call
	 * we should ignore it. */
	assert(attrs_in->valid_mask == ATTR_MODE);

	/* Truncate is not allowed */
	assert((openflags & FSAL_O_TRUNC) == 0);

	/* let open_by_handle fill in the attrs and the perm check  flag */
	result = kvsfs_open2_by_handle(obj_hdl, state, openflags, attrs_out,
				       caller_perm_check);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

out:
	T_EXIT0(result.major);
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
						       openflags,
						       attrs_out,
						       caller_perm_check);
		} else {
			result = kvsfs_open2_by_name(obj_hdl, name, new_obj,
						     state, openflags,
						     attrs_in, attrs_out,
						     caller_perm_check);
		}
	} else {
		switch (createmode) {
		case FSAL_UNCHECKED:
			if (name != NULL) {
				T_TRACE("%s", "Create Unchecked4");
				result = kvsfs_create_unchecked(obj_hdl,
								name,
								state,
								new_obj,
								openflags,
								attrs_in,
								attrs_out,
								caller_perm_check);
			} else {
				T_TRACE("%s", "Open Unchecked4");
				result = kvsfs_open_unchecked(obj_hdl,
							      state,
							      openflags,
							      attrs_in,
							      attrs_out,
							      caller_perm_check);
			}
			break;
		case FSAL_GUARDED:
			/* NFS Ganesha has already checked object existence.
			 * According to the RFC 4.0, the behavior in this case
			 * is the same as with Unchecked4 create.
			 */
			/*
			 * NOTE: Guarded4 is not used by the linux nfs client
			 * unless it uses NFS4.1 proto (which is not supported
			 * yet on our side).
			 * Therefore, this code path can be checked only
			 * by manually creating RPC requests or by
			 * using NFS4.1 linux nfs client.
			 * Moreover, Guarded4 is used by NFS4.1 only if
			 * the corresponding 4.1 session is persistent
			 * (see fs/nfs/nfs4proc.c, nfs4_open_prepare).
			 */
			T_TRACE("%s", "Create Guarded4");
			result = kvsfs_create_unchecked(obj_hdl,
							name,
							state,
							new_obj,
							openflags,
							attrs_in,
							attrs_out,
							caller_perm_check);
			break;
		case FSAL_EXCLUSIVE:
			/* Handles NFS4.0 version of open(O_CREAT | O_EXCL).
			 * Note: it is different from NFS4.1. O_EXCL.
			 */
			if (name == NULL) {
				/* We end up here if Ganesha detects
				 * that is was a re-transmit, i.e.
				 * Ganesha has done lookup() and checked
				 * the verifier and it matched.
				 * At this point we should just open
				 * the file.
				 */
				T_TRACE("%s", "Open Exclusive4");
				result = kvsfs_open_exclusive40(obj_hdl,
								state,
								openflags,
								attrs_in,
								attrs_out,
								caller_perm_check);
			} else {
				T_TRACE("%s", "Create Exclusive4");
				/* A normal O_EXCL create operation */
				result = kvsfs_create_exclusive40(obj_hdl,
								  name,
								  state,
								  new_obj,
								  openflags,
								  attrs_in,
								  attrs_out,
								  caller_perm_check,
								  verifier);
			}
			break;
		case FSAL_NO_CREATE:
			/* Impossible */
			assert(0);
			break;
		case FSAL_EXCLUSIVE_41:
		case FSAL_EXCLUSIVE_9P:
			result = fsalstat(ERR_FSAL_NOTSUPP, ENOTSUP);
			break;
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

static inline const char *state_type2str(enum state_type state_type)
{
	switch(state_type) {
	case STATE_TYPE_LOCK:
		return "LOCK";
	case STATE_TYPE_SHARE:
		return "SHARE";
	case STATE_TYPE_DELEG:
		return "DELEG";
	case STATE_TYPE_LAYOUT:
		return "LAYOUT";
	case STATE_TYPE_NLM_LOCK:
		return "NLM_LOCK";
	case STATE_TYPE_NLM_SHARE:
		return "NLM_SHARE";
	case STATE_TYPE_9P_FID:
		return "9P_FID";
	case STATE_TYPE_NONE:
		return "NONE";
	}

	return "<unknown>";
}

static fsal_status_t kvsfs_delete_on_close(struct fsal_obj_handle *obj_hdl)
{
	fsal_status_t result;
	struct kvsfs_state_fd *state_fd;
	struct kvsfs_fsal_obj_handle *obj;
	int rc;

	T_ENTER("%p", obj_hdl);

	PTHREAD_RWLOCK_wrlock(&obj_hdl->obj_lock);

	if (!fsal_obj_handle_is(obj_hdl, REGULAR_FILE)) {
		LogDebug(COMPONENT_FSAL, "Only a regular file can be closed");
		rc = 0;
		goto out;
	}

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	/* Check if there are any active SHARE states */
	if (kvsfs_fh_is_open(obj)) {
		T_TRACE("%s", "File is still open.");
		rc = 0;
		goto out;
	}

	/* TODO:PERF: Although this call checks st_nlink against zero,
	 * we can cache this value in the FH in order to avoid getattr()
	 * call inside this function.
	 */
	rc = kvsns_destroy_orphaned_file(obj->fs_ctx,
					 kvsfs_fh_to_ino(obj->handle));
	if (rc != 0) {
		LogCrit(COMPONENT_FSAL,
			"Failed to destroy file object %llu (%p)",
			(unsigned long long) *kvsfs_fh_to_ino(obj->handle),
			obj_hdl);
		goto out;
	}

out:
	PTHREAD_RWLOCK_unlock(&obj_hdl->obj_lock);
	T_EXIT0(rc);
	return fsalstat(posix2fsal_error(-rc), -rc);
}

static fsal_status_t kvsfs_close2(struct fsal_obj_handle *obj_hdl,
				  struct state_t *state)
{
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	struct kvsfs_state_fd *state_fd;
	struct kvsfs_fsal_obj_handle *obj;

	assert(state != NULL);

	T_ENTER(">>> (obj=%p, state=%p, type=%s)", obj_hdl, state,
		state_type2str(state->state_type));

	state_fd = container_of(state, struct kvsfs_state_fd, state);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	switch(state->state_type) {
	case STATE_TYPE_LOCK:
		/* A lock state has an associated open state which will be
		 * closed later.
		 */
		assert(kvsfs_file_state_invariant_closed(&state_fd->kvsfs_fd));
		break;

	case STATE_TYPE_SHARE:
		/* This state type is an actual open file. Let's close it. */
		assert(kvsfs_file_state_invariant_open(&state_fd->kvsfs_fd));
		result = kvsfs_file_state_close(&state_fd->kvsfs_fd, obj);
		if (FSAL_IS_SUCCESS(result)) {
			result = kvsfs_delete_on_close(obj_hdl);
		}
		break;

	case STATE_TYPE_DELEG:
		T_TRACE("Closing Delegation state: %d, %d",
			(int) state->state_data.deleg.sd_type,
			(int) state->state_data.deleg.sd_state);
		break;
	case STATE_TYPE_LAYOUT:
		/* Unsupported (yet) state types. */
		assert(0); // Non-implemented
		break;

	case STATE_TYPE_NLM_LOCK:
	case STATE_TYPE_NLM_SHARE:
	case STATE_TYPE_9P_FID:
	case STATE_TYPE_NONE:
		/* They will never be supported. */
		assert(0); // Unreachable
		break;
	}

	T_EXIT0(result.major);
	return result;
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
	uint64_t offset;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	fsal_status_t result;
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

	T_ENTER(">>> (%p, %p)", obj_hdl, read_arg->state);

	T_TRACE("READ: off=%llu, cnt=%llu, len=%llu",
		(unsigned long long) read_arg->offset,
		(unsigned long long) read_arg->iov_count,
		(unsigned long long) read_arg->iov[0].iov_len);

	/* NFS Ganesha uses only a single buffer so far. */
	assert(read_arg->iov_count == 1);

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	result = kvsfs_find_fd(read_arg->state, bypass, FSAL_O_READ, &fd);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	buffer = read_arg->iov[0].iov_base;
	buffer_size = read_arg->iov[0].iov_len;
	offset = read_arg->offset;

	nb_read = kvsns_read(obj->fs_ctx, &cred, &fd->kvsns_fd,
			     buffer, buffer_size, offset);
	if (nb_read < 0) {
		result = fsalstat(posix2fsal_error(-nb_read), -nb_read);
		goto out;
	}

	read_arg->io_amount = nb_read;

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
/* Atomicaly modifies the size of a file and its stats.
 * @see https://tools.ietf.org/html/rfc7530#section-16.32.
 * TODO:EOS-914: Not implemented yet.
 */
static fsal_status_t kvsfs_ftruncate(struct fsal_obj_handle *obj_hdl,
				     struct state_t *state, bool bypass,
				     struct stat *new_stat, int new_stat_flags)
{
	int rc;
	fsal_status_t result = fsalstat(ERR_FSAL_NO_ERROR, 0);
	struct kvsfs_file_state *fd = NULL;
	struct kvsfs_fsal_obj_handle *obj = NULL;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;

	T_ENTER0;

	assert(obj_hdl != NULL);
	assert((new_stat_flags & STAT_SIZE_SET) != 0);
	assert(obj_hdl->type == REGULAR_FILE);

	/* Check if there is an open state which has O_WRITE openflag
	 * set.
	 */
	result = kvsfs_find_fd(state, bypass, FSAL_O_WRITE, &fd);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}
	assert(fd != NULL);

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	rc = kvsns_truncate(obj->fs_ctx, &cred, &fd->kvsns_fd.ino,
			    new_stat, new_stat_flags);
	if (rc != 0) {
		result = fsalstat(posix2fsal_error(-rc), -rc);
		goto out;
	}

out:
	T_EXIT0(result.major);
	return result;
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
	uint64_t offset;
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	fsal_status_t result;
	ssize_t nb_write;
	void *buffer;
	size_t buffer_size;
	int rc;

	/* TODO: A temporary solution for keeping metadata (stat) consistent.
	 * The lock allows us to serialize all WRITE requests ensuring
	 * that stat is always updated in accordance with the amount
	 * of written data.
	 * See EOS-1424 for details.
	 */
	PTHREAD_RWLOCK_wrlock(&obj_hdl->obj_lock);

	/* We support only NFSv4 clients. kvsfs_open2 won't allow
	 * an NFSv3 client to open file, therefore we won't end up here
	 * in this case. Ergo, the following check is an assert precondition
	 * but not an input parameter check.
	 */
	assert(write_arg->state);
	/* Since we don't support NFSv3, we cannot handle WRITE_PLUS */
	assert(write_arg->info == NULL);

	T_ENTER(">>> (%p, %p)", obj_hdl, write_arg->state);
	T_TRACE("WRITE: stable=%d, off=%llu, cnt=%llu, len=%llu",
		(int) write_arg->fsal_stable,
		(unsigned long long) write_arg->offset,
		(unsigned long long) write_arg->iov_count,
		(unsigned long long) write_arg->iov[0].iov_len);

	/* So far, NFS Ganesha always sends only a single buffer in a FSAL.
	 * We can use this information for keeping write2 implementation
	 * simple, i.e. there is no need to implement pwritev-like call
	 * at the kvsns layer.
	 * The following pre-condition helps us to make sure that
	 * NFS Ganesha still uses the same scheme.
	 */
	assert(write_arg->iov_count == 1);

	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	result = kvsfs_find_fd(write_arg->state, bypass, FSAL_O_WRITE, &fd);
	if (FSAL_IS_ERROR(result)) {
		goto out;
	}

	buffer = write_arg->iov[0].iov_base;
	buffer_size = write_arg->iov[0].iov_len;
	offset = write_arg->offset;

	nb_write = kvsns_write(obj->fs_ctx, &cred, &fd->kvsns_fd,
			     buffer, buffer_size, offset);
	if (nb_write < 0) {
		result = fsalstat(posix2fsal_error(-nb_write), -nb_write);
		goto out;
	}

	write_arg->io_amount = nb_write;

	if (write_arg->fsal_stable) {
		nb_write = kvsns_fsync(obj->fs_ctx, &cred,
				       kvsfs_fh_to_ino(obj->handle));
		if (nb_write < 0) {
			write_arg->fsal_stable = false;
			result = fsalstat(posix2fsal_error(-nb_write), -nb_write);
			goto out;
		}
	}


	result = fsalstat(ERR_FSAL_NO_ERROR, 0);
out:
	T_EXIT0(result.major);
	PTHREAD_RWLOCK_unlock(&obj_hdl->obj_lock);
	done_cb(obj_hdl, result, write_arg, caller_arg);
}

/******************************************************************************/
/* FSAL.commit2 */
/* TODO:KVSNS does not support fsync() call, it always uses sync operations */
static fsal_status_t kvsfs_commit2(struct fsal_obj_handle *obj_hdl,
				   off_t offset, size_t len)
{
	T_ENTER0;
	(void) obj_hdl;
	(void) offset;
	(void) len;
	T_EXIT0(0);
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
	ops->unlink = kvsfs_remove;

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

	/* ops->lock_op2 is not implemented because this FSAl relies on
	 * NFS Ganesha byte-range lock handling.
	 * However, we might add lock_op2 implementation (and enable it in
	 * in the config) if we want to be able support locking of files
	 * outside of NFS Ganesha or just to get additional logging
	 * at the FSAL level.
	 * See EOS-34 for details.
	 */

	ops->lease_op2 = kvsfs_lease_op2;

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
