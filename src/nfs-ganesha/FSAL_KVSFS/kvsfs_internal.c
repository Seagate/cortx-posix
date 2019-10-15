/******************************************************************************/
#include "kvsfs_internal.h"
#include "kvsfs_export.h"

/******************************************************************************/
static inline bool kvsfs_fh_is_open(struct kvsfs_fsal_obj_handle *obj)
{
	static const struct fsal_share empty_share = { 0};
	return (memcmp(&empty_share, &obj->share, sizeof(empty_share)) != 0);
}

/******************************************************************************/
void kvsfs_construct_handle(struct fsal_export *export_base,
			     const kvsns_ino_t *ino,
			     const struct stat *stat,
			     struct fsal_obj_handle **obj)
{
	struct kvsfs_fsal_obj_handle *result;
	struct kvsfs_fsal_module *mymodule =
		container_of(op_ctx->fsal_export->fsal, struct kvsfs_fsal_module, fsal);

	result = gsh_calloc(1, sizeof(*result));

	result->handle[0] = (struct kvsfs_file_handle) { *ino };
	kvsfs_fsal_export_get_index(export_base, &result->fs_ctx);

	fsal_obj_handle_init(&result->obj_handle,
			     export_base,
			     posix2fsal_type(stat->st_mode));

	result->obj_handle.fsid = posix2fsal_fsid(stat->st_dev);
	result->obj_handle.fileid = stat->st_ino;

	result->obj_handle.obj_ops = &mymodule->handle_ops;

	*obj = &result->obj_handle;

	T_TRACE("Constructed handle: %p, INO: %d, FSID: %d:%d, FILEID: %d",
		*obj, (int) *ino,
		(int) result->obj_handle.fsid.major,
		(int) result->obj_handle.fsid.minor,
		(int) result->obj_handle.fileid);
}


/******************************************************************************/
/* INTERNAL */
fsal_status_t kvsfs_unlink_reg(struct fsal_obj_handle *dir_hdl,
				      struct fsal_obj_handle *obj_hdl,
				      const char *name)
{
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *parent;
	struct kvsfs_fsal_obj_handle *obj;
	int rc;

	parent = container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	rc = kvsns_destroy_link(parent->fs_ctx, &cred,
				&parent->handle->kvsfs_handle,
				&obj->handle->kvsfs_handle,
				name);
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
		rc = kvsns_destroy_file(parent->fs_ctx,
					&obj->handle->kvsfs_handle);
	}
	PTHREAD_RWLOCK_unlock(&obj->obj_handle.obj_lock);

out:
	if (rc != 0 ) {
		LogCrit(COMPONENT_FSAL, "Failed to unlink reg file"
			" %llu/%llu '%s'",
			(unsigned long long) parent->handle->kvsfs_handle,
			(unsigned long long) obj->handle->kvsfs_handle,
			name);
	}
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/* INTERNAL */
fsal_status_t kvsfs_rmsymlink(struct fsal_obj_handle *dir_hdl,
				     struct fsal_obj_handle *obj_hdl,
				     const char *name)
{
	kvsns_cred_t cred = KVSNS_CRED_INIT_FROM_OP;
	struct kvsfs_fsal_obj_handle *parent;
	struct kvsfs_fsal_obj_handle *obj;
	int rc;

	parent = container_of(dir_hdl, struct kvsfs_fsal_obj_handle, obj_handle);
	obj = container_of(obj_hdl, struct kvsfs_fsal_obj_handle, obj_handle);

	rc = kvsns2_unlink(parent->fs_ctx, &cred,
			  &parent->handle->kvsfs_handle,
			  &obj->handle->kvsfs_handle,
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
fsal_status_t kvsfs_rmdir(struct fsal_obj_handle *dir_hdl,
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

	rc = kvsns2_rmdir(parent->fs_ctx, &cred,
			  &parent->handle->kvsfs_handle,
			  (char *) name);
	if (rc != 0) {
		LogCrit(COMPONENT_FSAL, "Failed to rmdir name=%s, rc=%d",
			name, rc);
		goto out;
	}

out:
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
fsal_status_t kvsfs_file_state_open(struct kvsfs_file_state *state,
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
fsal_status_t kvsfs_file_state_close(struct kvsfs_file_state *state,
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
fsal_status_t kvsfs_find_fd(struct state_t *fsal_state,
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
/* Atomicaly modifies the size of a file and its stats.
 * @see https://tools.ietf.org/html/rfc7530#section-16.32.
 * TODO:EOS-914: Not implemented yet.
 */
fsal_status_t kvsfs_ftruncate(struct fsal_obj_handle *obj_hdl,
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

