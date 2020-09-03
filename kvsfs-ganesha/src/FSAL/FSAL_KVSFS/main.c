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

/* Implementation of FSAL module functions for CORTXFS library.  */

#include <limits.h>
#include <fsal_types.h>
#include <FSAL/fsal_init.h>
#include "fsal_internal.h"
#include <efs.h>


/* TODO:ATTR4_SEC_LABEL are not supported yet. */
#define KVSFS_SUPPORTED_ATTRIBUTES ((const attrmask_t) (ATTRS_POSIX | ATTR_ACL ))

#define KVSFS_LINK_MAX         EFS_MAX_LINK
#define FSAL_NAME              "CORTX-FS"
#define DBUS_INTERFACE_NAME    "org.ganesha.nfsd.config.fsal.cortxfs"

const char module_name[] = FSAL_NAME;

/* default parameters for EFS filesystem */
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
	.compute_readdir_cookie = false, /* NOTE: CORTXFS is not able to produce a stable
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
	CONF_ITEM_BOOL("pnfs_ds", true,
			kvsfs_fsal_module, fs_info.pnfs_ds),
	CONF_ITEM_BOOL("pnfs_mds", true,
			kvsfs_fsal_module, fs_info.pnfs_mds),        
	CONFIG_EOL
};

struct config_block kvsfs_param = {
	.dbus_interface_name = DBUS_INTERFACE_NAME,
	.blk_desc.name = FSAL_NAME,
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
	int rc = 0;
	struct kvsfs_fsal_module *kvsfs =
	    container_of(fsal_hdl, struct kvsfs_fsal_module, fsal);

	assert(kvsfs_config_ops());

	/* set default values */
	kvsfs->fs_info = default_kvsfs_info;

	/* load user values and exit upon encountering critical problems */
	rc = load_config_from_parse(config_struct, &kvsfs_param, kvsfs,
				    true, err_type);
	if (rc == -1) {
		if (!config_error_is_harmless(err_type)) {
			LogCrit(COMPONENT_FSAL, "Detected a major error"
				 "in the config file.");
			rc = -EINVAL;
			goto out;
		} else {
			LogMajor(COMPONENT_FSAL, "Detected an error"
				 "in the config file. Please check KVSFS "
				 "section for correctness of the settings.");
			rc = 0;
		}
	}

	/**
	 * TODO: In future when we move away from using ganesha.conf file
	 * for the pNFS params and start using the /etc/cortx/cortxfs.conf
	 * file, that change needs to be done from here
	 * Initialize KVSFS FSAL obj's pNFS module
	 */
	rc = kvsfs_pmds_ini(kvsfs, kvsfs_params);
	if (rc == -1) {
		LogCrit(COMPONENT_FSAL, "Failed to load pNFS config");
		rc = -EINVAL;
		goto out;
	}

	/* assign the final values to the base structure */
	fsal_hdl->fs_info = kvsfs->fs_info;

	/* print final values in the log */
	display_fsinfo(fsal_hdl);

	rc = efs_init(EFS_DEFAULT_CONFIG, kvsfs_config_ops());
	if (rc) {
		LogCrit(COMPONENT_FSAL, "Cannot initialize EFS (%d).", rc);
		goto out;
	}

out:
	LogInfo(COMPONENT_FSAL, "Initialized KVSFS config (%d).", rc);
	return fsalstat(posix2fsal_error(-rc), -rc);
}

/* KVSFS singleton */
struct kvsfs_fsal_module KVSFS;

static int kvsfs_unload(struct fsal_module *fsal_hdl);
static void kvsfs_load(void);

/*
 * What is the problem with dlopen/dlclose?
 * ------------------------------------------------
 *
 *  This section describes why we cannot use the default mechanism
 *  for load/unload of our FSAL.
 *  The default way of loading an FSAL is when NFS Ganesha
 *  calls dlopen() and the corresponding "constructor"
 *  (a function defined with __attribute__((constructor))) is getting
 *  called within the scope of this dlopen() call.
 *  The default way of unloading is when NFS Ganesha calls dlclose()
 *  and the corresponding function (defined with "destructor" attribute)
 *  is getting called within this call.
 *  These calls are not friendly to C TLS and libc:
 *
 * dlopen ->
 *	glibc takes a global lock (let's say g_mtx) and calls the ctor
 *		ctor kvsfs_load ->
 *			some 3d-party code is trying to
 *			use a C TLS variable (declared with __thread).
 *			glibc is trying initialize the variable
 *			and takes the global lock g_mtx.
 *			We got a deadlock.
 *
 *  dlopen/close also create problems with enabled/disabled jemalloc
 *  because it either involves different allocators (glibc and jemalloc)
 *  or the same troubles with this global lock for TLS.
 *  Either way, dlopen/dlclose should not be used for initialization/
 *  de-initialization of our library.
 *
 *
 * Solution for init/fini problem
 * ------------------------------
 *
 *  FSAL Initialization. When an FSAL was not able to initialize
 *  during dlopen() call, NFS Ganesha uses "fsal_init" symbol as
 *  a backup mechanism to initialize the FSAL: it looks it up and then
 *  calls it.
 *
 *  FSAL Finalization. NFS Ganesha provides the default implementation
 *  of the FSAL.unload callback. The function calls dlclose() which
 *  causes the "dtor" to be called. Since we don't want to use a dtor,
 *  we simply overwrite the default callback with our own function.
 *  Our function is called outside of dlclose(), so that we have no
 *  problems.
 *
 *  TODO: This overwriting of FSAL.unload leads to the situation where
 *  dlclose() is not called (as well as the ref count and the list of FSALs
 *  are not getting updated). Once we achieve the point where we are able to
 *  successfully terminate NFS Ganesha (without getting SIGABRT or SIGSEGV),
 *  we need to investigate if these things needs to be updated. For example,
 *  we can preserve the original callback pointer, and we can call it when
 *  our own "fini" call is done its work.
 */

/* This symbol has to be public (visible). NFS Ganesha uses it to load an FSAL
 * when the load-on-dlopen way does not work.
 * We do not use this dlopen+ctor trick because it plays very bad with
 * 3dparty libraries that use C TLS (for example, jemalloc or libuuid).
 * The same principle is applied to finalization. Previously we had
 * a combination of dclose+dtor which would finalize the FSAL when
 * dlclose is called. It also creates problems with C TLS. The solution
 * here is to use the FSAL.unload interface function let NFS Ganesha
 * call this function before dlclose is called.
 *
 * NOTE: Due to the collision between "fsal_init" symbol used
 * in NFS Ganesha ::load_fsal function and the function ::fsal_init
 * (see fsal_manager.c) that is declared in the global scope,
 * we cannot define our FSAL init function with the right
 * signature (void (*)(void)), and instead we have to use
 * the existing signature. It is safe as long as we don't
 * use the unused arguments.
 */
void *fsal_init(void *unused1, void *unused2)
{
	(void) unused1;
	(void) unused2;

	kvsfs_load();
	return NULL;
}

static void kvsfs_load(void)
{
	int rc;
	struct fsal_module *myself = &KVSFS.fsal;

	rc = register_fsal(myself, module_name,
			   FSAL_MAJOR_VERSION,
			   FSAL_MINOR_VERSION,
			   FSAL_ID_EXPERIMENTAL);
	if (rc) {
		LogCrit(COMPONENT_FSAL, "KVSFS FSAL module failed to "
			"register iself (%d).", rc);
		goto out;
	}

	/* Set up module operations */
	myself->m_ops.create_export = kvsfs_create_export;

	/* Set up module parameters */
	myself->m_ops.init_config = kvsfs_init_config;
	myself->m_ops.unload = kvsfs_unload;

	/* Set up pNFS operations */
	 myself->m_ops.fsal_pnfs_ds_ops = kvsfs_pnfs_ds_ops_init;
	 myself->m_ops.getdeviceinfo = kvsfs_getdeviceinfo;

	/* Initialize fsal_obj_handle for FSAL */
	kvsfs_handle_ops_init(&KVSFS.handle_ops);

out:
	LogDebug(COMPONENT_FSAL, "FSAL KVSFS initialized (%d)", rc);
	return;
}

static int kvsfs_unload(struct fsal_module *fsal_hdl)
{
	int rc = 0;

	assert(fsal_hdl == &KVSFS.fsal);

	rc = unregister_fsal(&KVSFS.fsal);
	if (rc) {
		LogCrit(COMPONENT_FSAL, "KVSFS FSAL module failed to"
			" unregister itself (%d).", rc);
		goto out;
	}

	rc = efs_fini();
	if (rc) {
		LogCrit(COMPONENT_FSAL, "efs_fini failed (%d)", rc);
		goto out;
	}

	rc = kvsfs_pmds_fini(&KVSFS);
	if (rc) {
		LogCrit(COMPONENT_FSAL, "kvsfs_pmds_fini() failed (%d)", rc);
		goto out;
	}

out:
	LogDebug(COMPONENT_FSAL, "FSAL KVSFS deinitialized (%d)", rc);
	return rc;
}

