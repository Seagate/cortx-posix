#ifndef KVSFS_HANDLE_H_
#define KVSFS_HANDLE_H_
/******************************************************************************/
/* File Handle API */
/******************************************************************************/

/* IMPORTs */
struct fsal_export;
struct fsal_obj_handle;
struct attrlist;
struct fsal_export;
struct kvsfs_fsal_index_context;
struct gsh_bufdesc;
enum state_type;
enum fsal_digesttype_t;
struct state_t;


/******************************************************************************/
/** Get export's Root handle by path. */
fsal_status_t kvsfs_lookup_path(struct fsal_export *exp_hdl,
			       const char *path,
			       struct fsal_obj_handle **handle,
			       struct attrlist *attrs_out);

/**Open a filesystem and get its descriptor. */
int kvsfs_export_to_kvsns_ctx(struct fsal_export *exp_hdl,
			      struct kvsfs_fsal_index_context **fs_ctx);

/** Create a file handle within the expecified NFS export. */
fsal_status_t kvsfs_create_handle(struct fsal_export *exp_hdl,
				 struct gsh_buffdesc *hdl_desc,
				 struct fsal_obj_handle **handle,
				 struct attrlist *attrs_out);

/** Create a file state */
struct state_t *kvsfs_alloc_state(struct fsal_export *exp_hdl,
				enum state_type state_type,
				struct state_t *related_state);

/** Destroy a file state */
void kvsfs_free_state(struct fsal_export *exp_hdl, struct state_t *state);

/** Get in-memory FH size. */
fsal_status_t kvsfs_extract_handle(struct fsal_export *exp_hdl,
				   enum fsal_digesttype_t in_type,
				   struct gsh_buffdesc *fh_desc,
				   int flags);

/** Initialize FH operations */
void kvsfs_handle_ops_init(struct fsal_obj_ops *ops);

/******************************************************************************/
#endif /* KVSFS_HANDLE_H_ */
