#ifndef KVSFS_EXPORT_H_
#define KVSFS_EXPORT_H_
/******************************************************************************/
#include <fsal_types.h> /* fsal_status_t */
/******************************************************************************/

/* IMPORTs */
struct fsal_module;
struct config_error_type;
struct fsal_up_vector;


/******************************************************************************/
/* EXPORTs */
struct kvsfs_fsal_export;
struct kvsfs_file_handle;
struct kvsfs_fsal_index_context;

/******************************************************************************/
/* Get pointer to RootInode object */
void kvsfs_fsal_export_get_rootfh(struct fsal_export *exp,
				   struct kvsfs_file_handle **inode);

/******************************************************************************/
/* Get pointer to Filesystem Index object */
void kvsfs_fsal_export_get_index(struct fsal_export *exp,
				 struct kvsfs_fsal_index_context **index);

/******************************************************************************/
/* Creates a new KVSFS export */
fsal_status_t kvsfs_create_export(struct fsal_module *fsal_hdl,
				  void *parse_node,
				  struct config_error_type *err_type,
				  const struct fsal_up_vector *up_ops);

/******************************************************************************/
#endif /* KVSFS_EXPORT_H_ */
