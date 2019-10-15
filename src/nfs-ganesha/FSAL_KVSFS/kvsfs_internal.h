#ifndef KVSFS_INTERNAL_H_
#define KVSFS_INTERNAL_H_
/******************************************************************************/
#include <gsh_list.h> /* container_of */
#include <fsal_convert.h> /* posix2fsal */
#include <FSAL/fsal_commonlib.h> /* FSAL methods */
#include <kvsns/kvsns.h> /* kvsns_ino_t */

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

struct kvsfs_fsal_index_context;

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

struct kvsfs_fsal_module {
	struct fsal_module fsal;
	struct fsal_staticfsinfo_t fs_info;
	struct fsal_obj_ops handle_ops;
};

/******************************************************************************/
/* Cred initialization */

static inline void kvsns_cred_from_op_ctx(kvsns_cred_t *out)
{
	assert(out);
	assert(op_ctx);

	out->uid = op_ctx->creds->caller_uid;
	out->gid = op_ctx->creds->caller_gid;
}

#define KVSNS_CRED_INIT_FROM_OP {		\
	.uid = op_ctx->creds->caller_uid,	\
	.gid = op_ctx->creds->caller_gid,	\
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
/* FH */
/******************************************************************************/
void kvsfs_construct_handle(struct fsal_export *export_base,
			     const kvsns_ino_t *ino,
			     const struct stat *stat,
			     struct fsal_obj_handle **obj);

/******************************************************************************/
/* Namespace */
fsal_status_t kvsfs_unlink_reg(struct fsal_obj_handle *dir_hdl,
				      struct fsal_obj_handle *obj_hdl,
				      const char *name);
fsal_status_t kvsfs_rmsymlink(struct fsal_obj_handle *dir_hdl,
				     struct fsal_obj_handle *obj_hdl,
				     const char *name);
fsal_status_t kvsfs_rmdir(struct fsal_obj_handle *dir_hdl,
				 struct fsal_obj_handle *obj_hdl,
				 const char *name);

/******************************************************************************/
/* IO */
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


fsal_status_t kvsfs_file_state_open(struct kvsfs_file_state *state,
					   const fsal_openflags_t openflags,
					   struct kvsfs_fsal_obj_handle *obj,
					   bool is_reopen);

fsal_status_t kvsfs_file_state_close(struct kvsfs_file_state *state,
					    struct kvsfs_fsal_obj_handle *obj);

fsal_status_t kvsfs_find_fd(struct state_t *fsal_state,
				   bool bypass,
				   fsal_openflags_t openflags,
				   struct kvsfs_file_state **fd);

fsal_status_t kvsfs_ftruncate(struct fsal_obj_handle *obj_hdl,
				     struct state_t *state, bool bypass,
				     struct stat *new_stat, int new_stat_flags);


/******************************************************************************/
/******************************************************************************/
#endif /* KVSFS_INTERNAL_H_ */
