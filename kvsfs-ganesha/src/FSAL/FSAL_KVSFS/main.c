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

/* Implementation of FSAL module functions for KVSNS library.  */

#include <limits.h>
#include <fsal_types.h>
#include <FSAL/fsal_init.h>
#include "fsal_internal.h"
#include <kvsns/kvsns.h> /* maxlink */
#include <efs.h>

/* TODO:ACL: ATTR_ACL and ATTR4_SEC_LABEL are not supported yet. */
#define KVSFS_SUPPORTED_ATTRIBUTES (ATTRS_POSIX)

#define KVSFS_LINK_MAX KVSNS_MAX_LINK

const char module_name[] = "KVSFS";

/* default parameters for KVSNS filesystem */
static const struct fsal_staticfsinfo_t default_kvsfs_info = {
	.maxfilesize	= INT64_MAX,		/*< maximum allowed filesize     */
	.maxlink	= KVSFS_LINK_MAX,	/*< maximum hard links on a file */
	.maxnamelen	= NAME_MAX,		/*< maximum name length */
	.maxpathlen	= PATH_MAX,		/*< maximum path length */
	.no_trunc	= true,			/*< is it errorneous when name>maxnamelen? */
	.chown_restricted = true,		/*< is chown limited to super-user. */
	.case_insensitive = false,		/*< case insensitive FS? */
	.case_preserving = true,		/*< FS preserves case? */
	.link_support	= true,			/*< FS supports hardlinks? */
	.symlink_support = true,		/*< FS supports symlinks? */
	.lock_support	= false,		/*< FS supports file locking? Intentionally disabled, see EOS-34. */
	.lock_support_async_block = false,	/*< FS supports blocking locks? Intentionally disabled, see EOS-34. */
	.named_attr	= false,		/*< FS supports named attributes. */ /* TODO */
	.unique_handles = true,			/*< Handles are unique and persistent. */
	.acl_support	= FSAL_ACLSUPPORT_ALLOW,	/*< what type of ACLs are supported */ /* TODO */
	.cansettime	= true,			/*< Is it possible to change file times using SETATTR. */
	.homogenous	= true,	/*< Are supported attributes the same for all objects of this filesystem. */
	.supported_attrs = KVSFS_SUPPORTED_ATTRIBUTES,	/*< If the FS is homogenous,
							  this indicates the set of
							  supported attributes. */
	.maxread = FSAL_MAXIOSIZE,	/*< Max read size */
	.maxwrite = FSAL_MAXIOSIZE,	/*< Max write size */
	.umask = 0,		/*< This mask is applied to the mode of created
				   objects */
	.auth_exportpath_xdev = false,	/*< This flag indicates weither
					   it is possible to cross junctions
					   for resolving an NFS export path. */
	.delegations = FSAL_OPTION_FILE_DELEGATIONS,	/*< fsal supports delegations */ /* TODO */
	.pnfs_mds = false,		/*< fsal supports file pnfs MDS */ /* TODO */
	.pnfs_ds = false,		/*< fsal supports file pnfs DS */ /* TODO */
	.fsal_trace = false,		/*< fsal trace supports */ /* TBD */
	.fsal_grace = false,		/*< fsal will handle grace */ /* TBD */
	.link_supports_permission_checks = true,
	.rename_changes_key = false,	/*< Handle key is changed across rename */
	.compute_readdir_cookie = false, /* NOTE: KVSNS is not able to produce a stable
					    offset for given filename
					    */
	.whence_is_name = false,		/* READDIR uses filename instead of offset
						TODO: update readdir implementation */
	.readdir_plus = false,	/*< FSAL supports readdir_plus */
	.expire_time_parent = -1, /*< Expiration time interval in
				       seconds for parent handle.
				       If FS gives information about parent
				       change for a directory with an upcall,
				       set this to -1. Else set it to some
				       positive value. Defaults to -1. */ /* TODO: implement upcall */
};

static struct config_item kvsfs_params[] = {
	CONF_ITEM_BOOL("link_support", true,
		       kvsfs_fsal_module, fs_info.link_support),
	CONF_ITEM_BOOL("symlink_support", true,
		       kvsfs_fsal_module, fs_info.symlink_support),
	CONF_ITEM_BOOL("cansettime", true,
		       kvsfs_fsal_module, fs_info.cansettime),
	CONF_ITEM_UI32("maxread", 512, FSAL_MAXIOSIZE, FSAL_MAXIOSIZE,
		       kvsfs_fsal_module, fs_info.maxread),
	CONF_ITEM_UI32("maxwrite", 512, FSAL_MAXIOSIZE, FSAL_MAXIOSIZE,
		       kvsfs_fsal_module, fs_info.maxwrite),
	CONF_ITEM_MODE("umask", 0,
		       kvsfs_fsal_module, fs_info.umask),
	CONF_ITEM_BOOL("auth_xdev_export", false,
		       kvsfs_fsal_module, fs_info.auth_exportpath_xdev),
	CONFIG_EOL
};

struct config_block kvsfs_param = {
	.dbus_interface_name = "org.ganesha.nfsd.config.fsal.kvsfs",
	.blk_desc.name = "KVSFS",
	.blk_desc.type = CONFIG_BLOCK,
	.blk_desc.u.blk.init = noop_conf_init,
	.blk_desc.u.blk.params = kvsfs_params,
	.blk_desc.u.blk.commit = noop_conf_commit
};

/* Init FSAL from config file */
static fsal_status_t kvsfs_init_config(struct fsal_module *fsal_hdl,
				     config_file_t config_struct,
				     struct config_error_type *err_type)
{
	struct kvsfs_fsal_module *kvsfs_me =
	    container_of(fsal_hdl, struct kvsfs_fsal_module, fsal);

	/* set default values */
	kvsfs_me->fs_info = default_kvsfs_info;

	/* load user values */
	(void) load_config_from_parse(config_struct,
				      &kvsfs_param,
				      kvsfs_me,
				      true,
				      err_type);

	if (!config_error_is_harmless(err_type)) {
		return fsalstat(ERR_FSAL_INVAL, 0);
	}

	fsal_hdl->fs_info = kvsfs_me->fs_info;
	/* print final values in the log */
	display_fsinfo(fsal_hdl);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/* KVSFS singleton */
struct kvsfs_fsal_module KVSFS;

MODULE_INIT void kvsfs_load(void)
{
	int retval;
	struct fsal_module *myself = &KVSFS.fsal;
/**
 * @todo: uncommenting this code block, causes nfsmount to fail.
 * Failure is in m0_clovis_init(), stuck on sem, called from
 * dstore_init()
 */
#if 0
	retval = efs_init(EFS_DEFAULT_CONFIG);
	if (retval != 0) {
		LogCrit(COMPONENT_FSAL,
			"efs init failed");
		return;
	}
#endif
	retval = register_fsal(myself, module_name,
			       FSAL_MAJOR_VERSION,
			       FSAL_MINOR_VERSION,
			       FSAL_ID_NO_PNFS);
	if (retval != 0) {
		LogCrit(COMPONENT_FSAL,
			"KVSFS FSAL module failed to register iself.");
		return;
	}

	/* Set up module operations */
	myself->m_ops.create_export = kvsfs_create_export;

	/* Set up module parameters */
	myself->m_ops.init_config = kvsfs_init_config;

	/* Set up pNFS opterations */
	/* myself->m_ops.fsal_pnfs_ds_ops = kvsfs_pnfs_ds_ops_init */;
	/* myself->m_ops.getdeviceinfo = kvsfs_getdeviceinfo */;

	/* Initialize fsal_obj_handle for FSAL */
	kvsfs_handle_ops_init(&KVSFS.handle_ops);

	LogDebug(COMPONENT_FSAL, "FSAL KVSFS initialized");
}

MODULE_FINI void kvsfs_unload(void)
{
	int retval;

	retval = unregister_fsal(&KVSFS.fsal);
	if (retval != 0) {
		LogCrit(COMPONENT_FSAL,
			"KVSFS FSAL module failed to unregister iself.");
		return;
	}

	LogDebug(COMPONENT_FSAL, "FSAL KVSFS deinitialized");
}

