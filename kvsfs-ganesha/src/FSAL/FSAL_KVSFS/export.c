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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * -------------
 */

/* export.c
 * KVSFS FSAL export object
 */

#include <stdint.h>
#include <common/log.h>
#include <config_parsing.h>
#include <fsal_types.h>
#include <FSAL/fsal_config.h>
#include <FSAL/fsal_commonlib.h>
#include "fsal_internal.h"
#include "kvsfs_methods.h"
#include <efs.h>

static struct config_item ds_array_params[] = {
	CONF_MAND_IP_ADDR("DS_Addr", "127.0.0.1",
			  kvsfs_pnfs_ds_parameter, ipaddr),
	CONF_MAND_UI16("DS_Port", 1024, UINT16_MAX, 2049,
		       kvsfs_pnfs_ds_parameter, ipport), /* default is nfs */
	CONFIG_EOL
};

static struct config_item pnfs_params[] = {
	CONF_MAND_UI32("Stripe_Unit", 8192, 1024*1024, 1024,
		       kvsfs_exp_pnfs_parameter, stripe_unit),
	CONF_ITEM_BOOL("pnfs_enabled", false,
		       kvsfs_exp_pnfs_parameter, pnfs_enabled),

	CONF_MAND_UI32("Nb_Dataserver", 1, 4, 1,
		       kvsfs_exp_pnfs_parameter, nb_ds),

	CONF_ITEM_BLOCK("DS1", ds_array_params,
			noop_conf_init, noop_conf_commit,
			kvsfs_exp_pnfs_parameter, ds_array[0]),

	CONF_ITEM_BLOCK("DS2", ds_array_params,
			noop_conf_init, noop_conf_commit,
			kvsfs_exp_pnfs_parameter, ds_array[1]),

	CONF_ITEM_BLOCK("DS3", ds_array_params,
			noop_conf_init, noop_conf_commit,
			kvsfs_exp_pnfs_parameter, ds_array[2]),

	CONF_ITEM_BLOCK("DS4", ds_array_params,
			noop_conf_init, noop_conf_commit,
			kvsfs_exp_pnfs_parameter, ds_array[3]),
	CONFIG_EOL

};


static int kvsfs_conf_pnfs_commit(void *node,
				  void *link_mem,
				  void *self_struct,
				  struct config_error_type *err_type);

static struct config_item export_params[] = {
	CONF_ITEM_NOOP("name"),
	CONF_ITEM_STR("kvsns_config", 0, MAXPATHLEN, NULL,
		      kvsfs_fsal_export, kvsns_config),
	CONF_ITEM_BLOCK("PNFS", pnfs_params,
			noop_conf_init, kvsfs_conf_pnfs_commit,
			kvsfs_fsal_export, pnfs_param),
	CONFIG_EOL
};


static struct config_block export_param = {
	.dbus_interface_name = "org.ganesha.nfsd.config.fsal.kvsfs-export",
	.blk_desc.name = "FSAL",
	.blk_desc.type = CONFIG_BLOCK,
	.blk_desc.u.blk.init = noop_conf_init,
	.blk_desc.u.blk.params = export_params,
	.blk_desc.u.blk.commit = noop_conf_commit
};


static void kvsfs_export_ops_init(struct export_ops *ops);

/* create_export
 * Create an export point and return a handle to it to be kept
 * in the export list.
 * First lookup the fsal, then create the export and then put the fsal back.
 * returns the export with one reference taken.
 */

fsal_status_t kvsfs_create_export(struct fsal_module *fsal_hdl,
				void *parse_node,
				struct config_error_type *err_type,
				const struct fsal_up_vector *up_ops)
{
	struct kvsfs_fsal_export *myself = NULL;
	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	int retval = 0;
	fsal_errors_t fsal_error = ERR_FSAL_INVAL;
	/* NOTE: "Path" in ganesha.conf is fsid for us */
	uint64_t fsid = strtoull(op_ctx->ctx_export->fullpath,
				 NULL, 0);

	myself = gsh_calloc(1, sizeof(struct kvsfs_fsal_export));

	fsal_export_init(&myself->export);
	kvsfs_export_ops_init(&myself->export.exp_ops);
	myself->export.up_ops = up_ops;

	retval = load_config_from_node(parse_node,
				       &export_param,
				       myself,
				       true,
				       err_type);
	if (retval != 0)
		goto errout;

	retval = efs_init(EFS_DEFAULT_CONFIG);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "Can't start EFS");
		goto errout;
	} else {
		LogEvent(COMPONENT_FSAL, "EFS API is running");
	}
	/* @todo FS mgmt work will replace/reuse this call here */
	retval = kvsns_set_fid(fsid);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "Can't set FSID:%"PRIu64" index",
			 fsid);
		goto errout;
	} else
		LogEvent(COMPONENT_FSAL, "FSID:%"PRIu64" FID set",
			 fsid);

	retval = fsal_attach_export(fsal_hdl, &myself->export.exports);
	if (retval != 0)
		goto err_locked;	/* seriously bad */
	myself->export.fsal = fsal_hdl;

	op_ctx->fsal_export = &myself->export;

	myself->pnfs_ds_enabled =
	    myself->export.exp_ops.fs_supports(&myself->export,
					    fso_pnfs_ds_supported) &&
					    myself->pnfs_param.pnfs_enabled;
	myself->pnfs_mds_enabled =
	    myself->export.exp_ops.fs_supports(&myself->export,
					    fso_pnfs_mds_supported) &&
					    myself->pnfs_param.pnfs_enabled;

	struct kvs_idx index;
	kvsns_fs_ctx_t fs_ctx = NULL;

	retval = kvsns_fs_open(fsid, &index);
	if (retval != 0) {
		LogMajor(COMPONENT_FSAL, "FS open failed, FSID:<%"PRIu64,
			 fsid);
		goto errout;
	}

	fs_ctx = index.index_priv;
	myself->index_context = fs_ctx;
	myself->fs_id = fsid;

	/* TODO:PORTING: pNFS support */
#if 0
	if (myself->pnfs_ds_enabled) {
		struct fsal_pnfs_ds *pds = NULL;

		status = fsal_hdl->m_ops.
			fsal_pnfs_ds(fsal_hdl, parse_node, &pds);
		if (status.major != ERR_FSAL_NO_ERROR)
			goto err_locked;

		/* special case: server_id matches export_id */
		pds->id_servers = op_ctx->export->export_id;
		pds->mds_export = op_ctx->export;

		if (!pnfs_ds_insert(pds)) {
			LogCrit(COMPONENT_CONFIG,
				"Server id %d already in use.",
				pds->id_servers);
			status.major = ERR_FSAL_EXIST;
			fsal_pnfs_ds_fini(pds);
			gsh_free(pds);
			goto err_locked;
		}

		LogInfo(COMPONENT_FSAL,
			"kvsfs_fsal_create: pnfs DS was enabled for [%s]",
			op_ctx->export->fullpath);
	}

	if (myself->pnfs_mds_enabled) {
		LogInfo(COMPONENT_FSAL,
			"kvsfs_fsal_create: pnfs MDS was enabled for [%s]",
			op_ctx->export->fullpath);
		export_ops_pnfs(&myself->export.exp_ops);
	}
#endif

	return fsalstat(ERR_FSAL_NO_ERROR, 0);

err_locked:
	if (myself->export.fsal != NULL)
		fsal_detach_export(fsal_hdl, &myself->export.exports);
errout:
	/* elvis has left the building */
	gsh_free(myself);

	return fsalstat(fsal_error, retval);

}

static int kvsfs_conf_pnfs_commit(void *node,
				  void *link_mem,
				  void *self_struct,
				  struct config_error_type *err_type)
{
	/* struct lustre_pnfs_param *lpp = self_struct; */

	/* Verifications/parameter checking to be added here */

	return 0;
}

/* export object methods
 */

static void export_release(struct fsal_export *exp_hdl)
{
	struct kvsfs_fsal_export *myself;

	myself = container_of(exp_hdl, struct kvsfs_fsal_export, export);

	fsal_detach_export(exp_hdl->fsal, &exp_hdl->exports);
	free_export_ops(exp_hdl);

	gsh_free(myself);		/* elvis has left the building */
}

/* statvfs-like call */
static fsal_status_t get_dynamic_info(struct fsal_export *exp_hdl,
				      struct fsal_obj_handle *obj_hdl,
				      fsal_dynamicfsinfo_t *infop)
{
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* kvsfs_export_ops_init
 * overwrite vector entries with the methods that we support
 */

void kvsfs_export_ops_init(struct export_ops *ops)
{
	ops->release = export_release;
	ops->lookup_path = kvsfs_lookup_path;
	ops->wire_to_host = kvsfs_extract_handle;
	ops->create_handle = kvsfs_create_handle;
	ops->get_fs_dynamic_info = get_dynamic_info;
	ops->alloc_state = kvsfs_alloc_state;
	ops->free_state = kvsfs_free_state;
}

