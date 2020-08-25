#ifndef _FSAL_INTERNAL_H
#define _FSAL_INTERNAL_H

#include "fsal.h" /* attributes */
#include "kvsfs_methods.h"

// forward declaration
struct kvsfs_pnfs_mds_ctx;

/* KVSFS FSAL module private storage
 */

struct kvsfs_fsal_module {
	struct fsal_module fsal;
	struct fsal_staticfsinfo_t fs_info;
	struct fsal_obj_ops handle_ops;
	/* pNFS related KVSFS FSAL's global config */
	struct kvsfs_pnfs_mds_ctx *mds_ctx;
};
/** KVSFS-related data for a file state object. */
struct kvsfs_file_state {
	/** The open and share mode etc. */
	fsal_openflags_t openflags;

	/** The CORTXFS file descriptor. */
	cfs_file_open_t cfs_fd;
};
fsal_status_t kvsfs_create_export(struct fsal_module *fsal_hdl,
				void *parse_node,
				struct config_error_type *err_type,
				const struct fsal_up_vector *up_ops);

void kvsfs_handle_ops_init(struct fsal_obj_ops *ops);
struct kvsfs_fsal_obj_handle {
	/* Base Handle */
	struct fsal_obj_handle obj_handle;

	/* CORTXFS Handle */
	struct cfs_fh *handle;

	/* TODO:EOS-3288
	 * This field will removed and replaced
	 * by a call to struct cfs_fh
	 */
	/* CORTXFS Context */
	struct cfs_fs *cfs_fs;

	/* Global state is disabled because we don't support NFv3. */
	/* struct kvsfs_file_state global_fd; */

	/* Share reservations */
	struct fsal_share share;
};

/* Returns a vtable that implements the ::cfs_endpoint_ops
 * interface for the NFS Ganesha (/etc/ganesha/ganesha.conf) config file.
 */
const struct cfs_endpoint_ops *kvsfs_config_ops(void);

/* linkage to the exports and handle ops initializers
 */

struct kvsfs_ds {
	struct fsal_ds_handle ds; /*< Public DS handle */
	struct kvsfs_file_handle wire; /*< Wire data */
	struct kvsfs_filesystem *kvsfs_fs; /*< Related kvsfs filesystem */
	bool connected; /*< True if the handle has been connected */
};

#if 0
static inline size_t kvsfs_sizeof_handle(struct kvsfs_file_handle *hdl)
{
	return (size_t) sizeof(struct kvsfs_file_handle);
}

/* the following variables must not be defined in fsal_internal.c */
#ifndef FSAL_INTERNAL_C

/* static filesystem info.
 * read access only.
 */
extern struct fsal_staticfsinfo_t global_fs_info;

#endif
#endif

/* KVSFS methods for pnfs
 */

int kvsfs_pmds_ini(struct kvsfs_fsal_module *kvsfs,
		   const struct config_item *kvsfs_params);
int kvsfs_pmds_fini(struct kvsfs_fsal_module *kvsfs);
int kvsfs_pmds_update_exp(struct kvsfs_fsal_module *kvsfs,
			  const struct kvsfs_fsal_export *exp_config);
nfsstat4 kvsfs_getdeviceinfo(struct fsal_module *fsal_hdl,
			      XDR *da_addr_body,
			      const layouttype4 type,
			      const struct pnfs_deviceid *deviceid);

size_t kvsfs_fs_da_addr_size(struct fsal_module *fsal_hdl);
void export_ops_pnfs(struct export_ops *ops);
void handle_ops_pnfs(struct fsal_obj_ops *ops);
void kvsfs_pnfs_ds_ops_init(struct fsal_pnfs_ds_ops *ops);

#endif /* _FSAL_INTERNAL_H */
