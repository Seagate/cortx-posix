/*
 * Filename:         fsal_internal.h
 * Description:      KVSFS FSAL module private storage
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.* 
 */


#ifndef _FSAL_INTERNAL_H
#define _FSAL_INTERNAL_H

#include <fsal.h> /* attributes */

/* KVSFS FSAL module private storage
 */

struct kvsfs_fsal_module {
	struct fsal_module fsal;
	struct fsal_staticfsinfo_t fs_info;
	struct fsal_obj_ops handle_ops;
};

fsal_status_t kvsfs_create_export(struct fsal_module *fsal_hdl,
				void *parse_node,
				struct config_error_type *err_type,
				const struct fsal_up_vector *up_ops);

void kvsfs_handle_ops_init(struct fsal_obj_ops *ops);

#if 0
#include  "fsal.h"

/* linkage to the exports and handle ops initializers
 */

void kvsfs_export_ops_init(struct export_ops *ops);

struct kvsfs_ds {
	struct fsal_ds_handle ds; /*< Public DS handle */
	struct kvsfs_file_handle wire; /*< Wire data */
	struct kvsfs_filesystem *kvsfs_fs; /*< Related kvsfs filesystem */
	bool connected; /*< True if the handle has been connected */
};

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

/* KVSFS methods for pnfs
 */

nfsstat4 kvsfs_getdeviceinfo(struct fsal_module *fsal_hdl,
			      XDR *da_addr_body,
			      const layouttype4 type,
			      const struct pnfs_deviceid *deviceid);

size_t kvsfs_fs_da_addr_size(struct fsal_module *fsal_hdl);
void export_ops_pnfs(struct export_ops *ops);
void handle_ops_pnfs(struct fsal_obj_ops *ops);
void kvsfs_pnfs_ds_ops_init(struct fsal_pnfs_ds_ops *ops);

#endif
#endif // 0
#endif
