/*
 * Filename:         mds.c
 * Description:      FSAL KVSFS's pNFS MetaData Server operations

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains KVSFS's pNFS MDS implementation
*/

/*
 *
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright © 2012 CohortFS, LLC.
 * Author: Adam C. Emerson <aemerson@linuxbox.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "config.h"

#include <assert.h>
#include <libgen.h>		/* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <mntent.h>
#include "gsh_list.h"
#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "FSAL/fsal_config.h"
#include "FSAL/fsal_commonlib.h"
#include "kvsfs_methods.h"
#include "nfs_exports.h"
#include "nfs_creds.h"
#include "pnfs_utils.h"
#include <stdbool.h>
#include <arpa/inet.h>

/* Wrappers for KVSFS handle debug traces. Enable this "#if" if you want
 * to get traces even without enabled DEBUG logging level, i.e. if you want to
 * see only debug logs from this module.
 */

#if 1
#define T_ENTER(_fmt, ...) \
	LogCrit(COMPONENT_PNFS, "T_ENTER " _fmt, __VA_ARGS__)

#define T_EXIT(_fmt, ...) \
	LogCrit(COMPONENT_PNFS, "T_EXIT " _fmt, __VA_ARGS__)

#define T_TRACE(_fmt, ...) \
	LogCrit(COMPONENT_PNFS, "T_TRACE " _fmt, __VA_ARGS__)
#else
#define T_ENTER(_fmt, ...) \
	LogDebug(COMPONENT_PNFS, "T_ENTER " _fmt, __VA_ARGS__)

#define T_EXIT(_fmt, ...) \
	LogDebug(COMPONENT_PNFS, "T_EXIT " _fmt, __VA_ARGS__)

#define T_TRACE(_fmt, ...) \
	LogDebug(COMPONENT_PNFS, "T_TRACE " _fmt, __VA_ARGS__)
#endif

#define T_ENTER0 T_ENTER(">>> %s", "()");
#define T_EXIT0(__rcval)  T_EXIT("<<< rc=%d", __rcval);

extern struct kvsfs_fsal_module KVSFS;

/* FSAL_KVSFS currently supports only LAYOUT4_NFSV4_1_FILES */
#define SUPPORTED_LAYOUT_TYPE LAYOUT4_NFSV4_1_FILES

/*  Only 4MB as of now */
#define SUPPORTED_LAYOUT_BLOCK_SIZE 0x400000U

/* Since current clients only support 1, that's what we'll use. */
#define SUPPORTED_LAYOUT_MAX_SEGMENTS 1U

#define SUPPORTED_LAYOUT_LOC_BODY_SIZE 0x100U

#define SUPPORTED_LAYOUT_DA_ADDR_SIZE 0x1400U

#define KVSFS_PNFS_MIN_STRIPE_COUNT 1U
#define KVSFS_PNFS_MAX_HOST_COUNT 3U

/**
 * 1. If kvsfs_fsal_obj_handle.pnfs_role == PNFS_DISABLED:
 * 	kvsfs_fsal_obj_handle.mds_ctx == NULL
 *
 * 2. If kvsfs_fsal_obj_handle.pnfs_role == PNFS_MDS:
 * 	kvsfs_fsal_obj_handle.mds_ctx != NULL
 * 	kvsfs_pnfs_mds_ctx.num_ds >= 0
 * 	kvsfs_pnfs_mds_ctx.num_ds reflects no. of nodes in
 * 		kvsfs_pnfs_mds_ctx.mds_ds_list
 *
 * 3. If kvsfs_fsal_obj_handle.pnfs_role == PNFS_DS:
 * 	kvsfs_fsal_obj_handle.mds_ctx == NULL
 *
 * 4. If kvsfs_fsal_obj_handle.pnfs_role == PNFS_BOTH:
 * 	conditions for 2 applicable, addition to that:
 * 	kvsfs_pnfs_mds_ctx.mds_ds_list must have atleast one elem, i.e. self
 */

enum kvsfs_pNFS_server_role {
	KVSFS_PNFS_DISABLED = 0,
	KVSFS_PNFS_DS,
	KVSFS_PNFS_MDS,
	KVSFS_PNFS_BOTH
};

struct kvsfs_pnfs_ds_addr {
	uint16_t ds_addr_id;
	uint16_t ds_addr_ref;
	in_port_t ds_ip_port;
	sockaddr_t ds_ip_addr;
	LIST_ENTRY(kvsfs_pnfs_ds_addr) ds_addr_link;
};

/**
 * TODO: when layout states will be maintained in future and dynamic DS list
 * will be supported, a DS entry should be protected with 'ds_ref' from
 * deletion. Same is applicable for 'ds_addr_ref' as well.
 * 'ds_id, ds_ref, ds_addr_id and ds_addr_ref' are for future use.
 */
struct kvsfs_pnfs_ds_info {
	uint16_t ds_id;
	uint16_t ds_ref;
	LIST_HEAD(kvsfs_pnfs_ds_addr_list, kvsfs_pnfs_ds_addr) ds_addr_list;
	LIST_ENTRY(kvsfs_pnfs_ds_info) ds_link;
};

struct kvsfs_pnfs_mds_info {
	uint32_t stripe_unit;
	LIST_HEAD(kvsfs_pnfs_ds_info_list, kvsfs_pnfs_ds_info) mds_ds_list;
	uint16_t num_ds;
	struct kvsfs_pnfs_ds_info *mds_next_ds;
};

struct kvsfs_pnfs_mds_ctx {
	uint32_t pnfs_role;
	pthread_mutex_t mds_mutex;
	struct kvsfs_pnfs_mds_info *mds;
};


/**
 * @brief Initialize the KVSFS FSAL's global pNFS config, must be called only
 * once while FSAL loads/starts
 *
 * @param[in]  kvsfs         KVSFS FSAL's global context
 * @param[in]  kvsfs_params  Config from cortx.conf
 * @param[out] kvsfs         pNFS config created and updated if available
 *
 * @return 0 if success, else negative error value
 */


int kvsfs_pmds_ini(struct kvsfs_fsal_module *kvsfs,
		   const struct config_item *kvsfs_params)
{
	int rc = 0;
	struct kvsfs_pnfs_mds_ctx *mds_ctx;

	dassert(kvsfs != NULL);
	dassert(kvsfs_params != NULL);
	dassert(kvsfs->mds_ctx == NULL);

	/** 
	 * TODO: When the pNFS config is moved to cortx.conf file, process
	 * kvsfs_params here and update kvsfs->pnfs_role and
	 * kvsfs->mds_ctx. Until then, just initialize kvsfs->mds_ctx.
	 * In future, initialize only if pnfs_role > KVSFS_PNFS_DS.
	 */
	mds_ctx = gsh_calloc(1, sizeof(*mds_ctx));
	if (mds_ctx == NULL) {
		LogCrit(COMPONENT_PNFS,
			"gsh_calloc failed: %u", sizeof(*mds_ctx->mds));
		rc = -ENOMEM;
		goto out;
	}

	mds_ctx->pnfs_role = KVSFS_PNFS_DISABLED;
	mds_ctx->mds = gsh_calloc(1, sizeof(struct kvsfs_pnfs_mds_info));
	if (mds_ctx->mds == NULL) {
		LogCrit(COMPONENT_PNFS,
			"gsh_calloc failed: %u", sizeof(*mds_ctx->mds));
		rc = -ENOMEM;
		goto out;
	}

	rc = pthread_mutex_init(&mds_ctx->mds_mutex, NULL);
	if (rc != 0) {
		LogCrit(COMPONENT_PNFS,
			"pthread_mutex_init failed: %x", rc);
		goto out;
	}

	LIST_INIT(&mds_ctx->mds->mds_ds_list);

out:
	if (rc != 0) {
		if (mds_ctx) {
			gsh_free(mds_ctx->mds);
		}
		gsh_free(mds_ctx);
	} else {
		kvsfs->mds_ctx = mds_ctx;
	}

	return rc;
}

/**
 * @brief Cleanup the MDS context (private helper)
 *
 * @param[in]  mds       MDS info
 * @return void
 */

static void kvsfs_pmds_cleanup(struct kvsfs_pnfs_mds_info *mds)
{
	struct kvsfs_pnfs_ds_info *ds;
	struct kvsfs_pnfs_ds_addr *ds_addr;

	dassert(mds != NULL);

	while (!LIST_EMPTY(&mds->mds_ds_list)) {
		ds = LIST_FIRST(&mds->mds_ds_list);
		/**
		 * TODO: Add support for ds_ref
		 * When a DS is being removed, then need to check if there are
		 * any active layouts using this DS< then the mapping needs to
		 * be updated and associated layouts needs to be recalled.
		 */
		LIST_REMOVE(ds, ds_link);
		while (!LIST_EMPTY(&ds->ds_addr_list)) {
			ds_addr = LIST_FIRST(&ds->ds_addr_list);
			// TODO: Add support for ds_addr_refds_addr_ref
			// Same as prev. one
			LIST_REMOVE(ds_addr, ds_addr_link);
			gsh_free(ds_addr);
		}
		gsh_free(ds);
	}

	gsh_free(mds);
}

/**
 * @brief De-initialize the KVSFS FSAL's global pNFS config, must be called only
 * once while FSAL unloads/stops
 *
 * @param[in]  kvsfs         KVSFS FSAL's global context
 * @param[out] kvsfs         pNFS config and context removed
 *
 * @return 0 if success, else negative error value
 */

int kvsfs_pmds_fini(struct kvsfs_fsal_module *kvsfs)
{
	int rc = 0;

	dassert(kvsfs != NULL);

	rc = pthread_mutex_lock(&kvsfs->mds_ctx->mds_mutex);
	if (rc != 0) {
		LogCrit(COMPONENT_PNFS, "pthread_mutex_lock failed: %x", rc);
		goto out;
	}

	kvsfs_pmds_cleanup(kvsfs->mds_ctx->mds);

	rc = pthread_mutex_unlock(&kvsfs->mds_ctx->mds_mutex);
	if (rc != 0) {
		LogCrit(COMPONENT_PNFS, "pthread_mutex_unlock failed: %x", rc);
		goto out;
	}

	rc = pthread_mutex_destroy(&kvsfs->mds_ctx->mds_mutex);
	if (rc != 0) {
		LogCrit(COMPONENT_PNFS, "pthread_mutex_destroy failed: %x", rc);
		goto out;
	}

	gsh_free(kvsfs->mds_ctx);

out:
	return rc;
}


/**
 * @brief Load pNFS config from export entries/override with new entries
 *
 * Until the global config support is added, we keep using the per-export
 * config as of now, but only take the pNFS config from the very first export.
 * Adding new exports/endpoints will override previous config and will cause
 * run-time update of the in-memory pNFS config. Until HA's multi-node support
 * is integrated, this approach can be used temporarily to simulate add-delete
 * multiple DS dynamically.
 * However please note that this support is not complete without implementing
 * the references (i.e. ds_ref and ds_addr_refds_addr_ref).
 * Note: Update will be cancelled if configuration error is encountered.
 * Duplicate/No-op update validation is not available yet.
 *
 * @param[in]  kvsfs         KVSFS FSAL's global context
 * @param[in]  exp_config    export config to use for new pNFS config
 * @param[out] kvsfs         pNFS config and context modified 
 *
 * @return 0 if success, else negative error value
 *
 */

int kvsfs_pmds_update_exp(struct kvsfs_fsal_module *kvsfs,
			  const struct kvsfs_fsal_export *exp_config)
{
	int rc = 0;
	int idx = 0;
	uint32_t local_pnfs_role;
	struct kvsfs_pnfs_mds_info *local_mds = NULL;
	struct kvsfs_pnfs_ds_info *ds;
	struct kvsfs_pnfs_ds_addr *ds_addr;

	dassert(kvsfs != NULL);
	dassert(exp_config != NULL);

	local_pnfs_role = KVSFS_PNFS_DISABLED;

	if (exp_config->pnfs_param.pnfs_enabled) {
		if ((exp_config->pnfs_ds_enabled) &&
		    (exp_config->pnfs_mds_enabled)) {
			local_pnfs_role = KVSFS_PNFS_BOTH;
		} else if (exp_config->pnfs_mds_enabled) {
			local_pnfs_role = KVSFS_PNFS_MDS;
		} else if (exp_config->pnfs_ds_enabled) {
			local_pnfs_role = KVSFS_PNFS_DS;
		}
	}

	if (local_pnfs_role > KVSFS_PNFS_DS) {
		local_mds = gsh_calloc(1, sizeof(*local_mds));
		if (local_mds == NULL) {
			LogCrit(COMPONENT_PNFS,
				"gsh_calloc failed: %u",
				sizeof(*local_mds));
			rc = -ENOMEM;
			goto out;
		}

		local_mds->stripe_unit =
			exp_config->pnfs_param.stripe_unit;

		LIST_INIT(&local_mds->mds_ds_list);
		local_mds->num_ds = 0;

		for(idx = 0; idx < exp_config->pnfs_param.nb_ds; idx++) {
			ds = gsh_calloc(1, sizeof(*ds));
			if (ds == NULL) {
				LogCrit(COMPONENT_PNFS,
					"gsh_calloc failed: %u",
					sizeof(*ds));
				rc = -ENOMEM;
				goto out;
			}
			/**
			 * TODO: Once dynamic add/remove DS support is added
			 * either via temp. config file or via HA, we will need
			 * to design how to update and maintain ds_id
			 */
			ds->ds_id = idx;
			LIST_INIT(&ds->ds_addr_list);

			// per DS, only one addr as of now
			// TODO: support for multiple addresses
			ds_addr = gsh_calloc(1, sizeof(*ds_addr));
			if (ds_addr == NULL) {
				LogCrit(COMPONENT_PNFS,
					"gsh_calloc failed: %u",
					sizeof(*ds_addr));
				rc = -ENOMEM;
				goto out;
			}
			ds_addr->ds_addr_id =
				exp_config->pnfs_param.ds_array[idx].id;
			ds_addr->ds_ip_port =
				exp_config->pnfs_param.ds_array[idx].ipport;
			ds_addr->ds_ip_addr =
				exp_config->pnfs_param.ds_array[idx].ipaddr;

			LIST_INSERT_HEAD(&ds->ds_addr_list, ds_addr,
					 ds_addr_link);

			LIST_INSERT_HEAD(&local_mds->mds_ds_list, ds,
					 ds_link);
			local_mds->num_ds++;
		}

		rc = pthread_mutex_lock(&kvsfs->mds_ctx->mds_mutex);
		if (rc != 0) {
			LogCrit(COMPONENT_PNFS,
				"pthread_mutex_lock failed: %x", rc);
			goto out;
		}

		// delete the current config and commit the change
		kvsfs_pmds_cleanup(kvsfs->mds_ctx->mds);

		kvsfs->mds_ctx->pnfs_role = local_pnfs_role;
		kvsfs->mds_ctx->mds = local_mds; 

		rc = pthread_mutex_unlock(&kvsfs->mds_ctx->mds_mutex);
		if (rc != 0) {
			LogCrit(COMPONENT_PNFS,
				"pthread_mutex_unlock failed: %x", rc);
			goto out;
		}
	}

out:
	if (rc != 0) {
		kvsfs_pmds_cleanup(local_mds);
	}

	return rc;
}

/**
 * @brief Internal handler for kvsfs_getdeviceinfo()
 * Get information about a pNFS layout's stripe to device mapping.
 * This function always returns only one stripe as of now, i.e. only one map
 * of a layout to a data server. Each call to this handler will traverse the
 * global DS list of this MDS and will assign a DS with its IP addresses in a
 * round robin manner. As of now, it does not support load balancing,
 * in-memory or persistent device map, multiple stripes etc.
 *
 * @param[in]  mds_ctx       Global KVSFS FSAL's pNFS MDS context
 * @param[in]  deviceid	     caller passed device id
 * @param[out] stripes_nmemb array 'stripes' no. of entries, 1 as of now
 * @param[out] stripes       array of stripes
 * @param[out] ds_count      How many DS are being returned, 1 as of now
 * @param[out] hosts_nmemb   per DS number of hosts, denotes the array length
 * @param[out] hosts	     per DS array of hosts
 *
 * @return Valid error codes in RFC 5661, p. 365.
 */

nfsstat4
kvsfs_pmds_getdevinfo_internal(struct kvsfs_pnfs_mds_ctx *mds_ctx,
			       const struct pnfs_deviceid *deviceid,
			       uint32_t *stripes_nmemb,
			       uint32_t **stripes,
			       uint32_t *ds_count,
			       uint32_t *hosts_nmemb,
			       fsal_multipath_member_t **hosts)
{
	nfsstat4 nfs_status = NFS4_OK;
	struct kvsfs_pnfs_ds_info **ds;
	struct kvsfs_pnfs_ds_addr *ds_addr;
	unsigned long dsaddr = INADDR_NONE;
	struct sockaddr_in6 *addr;
	uint32_t stripes_count = KVSFS_PNFS_MIN_STRIPE_COUNT;
	uint32_t idx = 0;
	fsal_multipath_member_t hosts_local[KVSFS_PNFS_MAX_HOST_COUNT] = {0};
	int rc = 0;

	dassert(deviceid != NULL);
	dassert(stripes_nmemb != NULL);
	dassert(*stripes == NULL);
	dassert(ds_count != NULL);
	dassert(hosts_nmemb != NULL);
	dassert(*hosts == NULL);

	/**
	 * Only pNFS Meta Data Server is allowed to execute this
	 */
	if (mds_ctx->pnfs_role < KVSFS_PNFS_MDS) {
		LogCrit(COMPONENT_PNFS, "incorrect role: %x",
			mds_ctx->pnfs_role);
		nfs_status = NFS4ERR_NOTSUPP;
		goto out;
	}

	dassert(mds_ctx->mds != NULL);

	/**
	 * Only the deviceid.fsal_id is being used currently
	 */
	if (deviceid->fsal_id != FSAL_ID_EXPERIMENTAL) {
		LogCrit(COMPONENT_PNFS,
				"deviceid.fsal_id incorrect: %x",
				deviceid->fsal_id);
		nfs_status = NFS4ERR_INVAL;
		goto out;
	}

	*stripes = gsh_calloc(stripes_count, sizeof (uint32_t));
	if (*stripes == NULL) {
		LogCrit(COMPONENT_PNFS,
				"gsh_calloc failed: %u",
				stripes_count * sizeof (uint32_t));
		nfs_status = NFS4ERR_SERVERFAULT;
		goto out;
	}

	rc = pthread_mutex_lock(&mds_ctx->mds_mutex);
	dassert(rc == 0);

	ds = &mds_ctx->mds->mds_next_ds;

	if (*ds == NULL) {
		// get an elem
		if (mds_ctx->mds->num_ds == 0) {
			// consistency check
			dassert(LIST_EMPTY(
				&mds_ctx->mds->mds_ds_list) != True);
			// atleast the self entry is expected for KVSFS_PNFS_BOTH
			dassert(mds_ctx->pnfs_role != KVSFS_PNFS_BOTH);
			LogWarn(COMPONENT_PNFS, "No DS for role: %x",
				mds_ctx->pnfs_role);
			nfs_status = NFS4ERR_NOENT;
			goto unlock_out;
		}
		*ds = LIST_FIRST(&mds_ctx->mds->mds_ds_list);
	} else {
		// try to use the next one if available
		*ds = LIST_NEXT(*ds, ds_link);
		if (*ds == NULL) {
			*ds = LIST_FIRST(&mds_ctx->mds->mds_ds_list);
		}
	}

	LIST_FOREACH(ds_addr, &(*ds)->ds_addr_list, ds_addr_link) {
		/* Ganesha parsing code in config_parsing does not store the
		 * AF family value. Check if this is a IPV4 mapped IPV6 address,
		 * if not, this is a V4 address.
		 */
		addr = (struct sockaddr_in6 *) (&ds_addr->ds_ip_addr);
		if(IN6_IS_ADDR_V4MAPPED(&addr->sin6_addr)) {
			/* A IPv4 mapped IPv6 address, consists of 
			 * 80 "0" bits, followed by 16 "1" bits
			 * followed by 32 bit IPv4 address
			 */
			dsaddr = ((struct in_addr *)
					(addr->sin6_addr.s6_addr+12))->s_addr;
			LogDebug(COMPONENT_PNFS,
					"advertises DS addr=%u.%u.%u.%u port=%u",
					(ntohl(dsaddr) & 0xFF000000) >> 24,
					(ntohl(dsaddr) & 0x00FF0000) >> 16,
					(ntohl(dsaddr) & 0x0000FF00) >> 8,
					(unsigned int)ntohl(dsaddr) & 0x000000FF,
					(unsigned short)(ds_addr->ds_ip_port));
		} else {
			// This should be a V4 address
			LogDebug(COMPONENT_PNFS, "\n This is a v4 address\n");
			dsaddr = ((struct sockaddr_in *)
					(&ds_addr->ds_ip_addr))->sin_addr.s_addr;
		}

		// This should not happen
		dassert(idx != KVSFS_PNFS_MAX_HOST_COUNT);

		hosts_local[idx].proto = IPPROTO_TCP;
		hosts_local[idx].addr = ntohl(dsaddr);
		hosts_local[idx++].port = ds_addr->ds_ip_port;
	}

unlock_out:
	pthread_mutex_unlock(&mds_ctx->mds_mutex);
	dassert(rc == 0);

out:
	if (nfs_status != NFS4_OK) {
		gsh_free(stripes);
		gsh_free(hosts);
	} else {
		*hosts = gsh_calloc(idx, sizeof(**hosts));
		if (*hosts == NULL) {
			LogCrit(COMPONENT_PNFS,
				"gsh_calloc failed: %u",
				 idx * sizeof(**hosts));
			nfs_status = NFS4ERR_SERVERFAULT;
			goto out;
		}

		memcpy(*hosts, hosts_local, idx * sizeof(**hosts));
		*stripes_nmemb = stripes_count;
		// as of now, stripe:ds == 1:1
		*ds_count = stripes_count;
		*hosts_nmemb = idx;
	}

	return nfs_status;
}

/**
 * @brief Get layout types supported by this FSAL
 * Note: This is Filesystem wide
 *
 * We just return a pointer to the single type and set the count to 1.
 *
 * @param[in]  export_pub Public export handle
 * @param[out] count      Number of layout types in array
 * @param[out] types      Static array of layout types that must not be
 *                        freed or modified and must not be dereferenced
 *                        after export reference is relinquished
 */
static void
kvsfs_fs_layouttypes(struct fsal_export *export_hdl,
		      int32_t *count,
		      const layouttype4 **types)
{
	static layouttype4 type = SUPPORTED_LAYOUT_TYPE;
	*types = &type;
	*count = 1;
}

/**
 * @brief Get layout block size for export
 *
 * This function just returns the KVSFS default.
 *
 * @param[in] export_pub Public export handle
 *
 * @return 4 MB.
 */
static uint32_t
kvsfs_fs_layout_blocksize(struct fsal_export *export_pub)
{
	return SUPPORTED_LAYOUT_BLOCK_SIZE;
}

/**
 * @brief Maximum number of segments we will use
 *
 * Since current clients only support 1, that's what we'll use.
 *
 * @param[in] export_pub Public export handle
 *
 * @return 1
 */
static uint32_t
kvsfs_fs_maximum_segments(struct fsal_export *export_pub)
{
	return SUPPORTED_LAYOUT_MAX_SEGMENTS;
}

/**
 * @brief Size of the buffer needed for a loc_body
 *
 * Just a handle plus a bit.
 *
 * @param[in] export_pub Public export handle
 *
 * @return Size of the buffer needed for a loc_body
 */
static size_t
kvsfs_fs_loc_body_size(struct fsal_export *export_pub)
{
	return SUPPORTED_LAYOUT_LOC_BODY_SIZE;
}

/**
 * @brief Size of the buffer needed for a ds_addr
 *
 * This one is huge, due to the striping pattern.
 *
 * @param[in] export_pub Public export handle
 *
 * @return Size of the buffer needed for a ds_addr
 */
size_t kvsfs_fs_da_addr_size(struct fsal_module *fsal_hdl)
{
	return SUPPORTED_LAYOUT_DA_ADDR_SIZE;
}



/**
 * @brief Get information about a pNFS device
 *
 * When this function is called, the FSAL should write device
 * information to the @c da_addr_body stream.
 *
 * @param[in]  fsal_hdl     FSAL module
 * @param[out] da_addr_body Stream we write the result to
 * @param[in]  type         Type of layout that gave the device
 * @param[in]  deviceid     The device to look up
 *
 * @return Valid error codes in RFC 5661, p. 365.
 */


nfsstat4 kvsfs_getdeviceinfo(struct fsal_module *fsal_hdl,
			      XDR *da_addr_body,
			      const layouttype4 type,
			      const struct pnfs_deviceid *deviceid)
{
	uint32_t stripes_nmemb;
	uint32_t *stripes = NULL;
	uint32_t ds_count;
	uint32_t hosts_nmemb;
	fsal_multipath_member_t *hosts = NULL;
	/* NFSv4 status code */
	nfsstat4 nfs_status = NFS4_OK;
	int idx = 0;

	T_ENTER(">>> (%p, %p)", fsal_hdl, deviceid);
	LogDebug(COMPONENT_PNFS,">> ENTER kvsfs_getdeviceinfo\n");

	/* Sanity check on type */
	if (type != LAYOUT4_NFSV4_1_FILES) {
		LogCrit(COMPONENT_PNFS, "Unsupported layout type: %x", type);
		nfs_status = NFS4ERR_UNKNOWN_LAYOUTTYPE;
		T_EXIT0(nfs_status);
		goto out;
	}

	LogDebug(COMPONENT_PNFS, "device_id %u/%u/%u %lu",
		 deviceid->device_id1, deviceid->device_id2,
		 deviceid->device_id4, deviceid->devid);

	nfs_status = kvsfs_pmds_getdevinfo_internal(KVSFS.mds_ctx, deviceid,
						    &stripes_nmemb,
						    &stripes,
						    &ds_count,
						    &hosts_nmemb,
						    &hosts);
	if (nfs_status != NFS4_OK) {
		LogCrit(COMPONENT_PNFS,
			"kvsfs_pmds_getdevinfo_internal failed: %x",
			nfs_status);
		T_EXIT0(nfs_status);
		goto out;
	}

	if (!inline_xdr_u_int32_t(da_addr_body, &stripes_nmemb)) {
		LogCrit(COMPONENT_PNFS,
			"Failed to encode length of stripe_indices array: %"
			PRIu32 ".",
			stripes_nmemb);
		nfs_status = NFS4ERR_SERVERFAULT;
		T_EXIT0(nfs_status);
		goto out;
	}

	for (idx = 0; idx < stripes_nmemb; idx++) {
		if (!inline_xdr_u_int32_t(da_addr_body, &stripes[idx])) {
			LogCrit(COMPONENT_PNFS,
				"Failed to encode OSD for stripe %u.%u",
				idx, stripes[idx]);
			nfs_status = NFS4ERR_SERVERFAULT;
			T_EXIT0(nfs_status);
			goto out;
		}
	}

	if (!inline_xdr_u_int32_t(da_addr_body, &ds_count)) {
		LogCrit(COMPONENT_PNFS,
			"Failed to encode ds_count: %u", ds_count);
		nfs_status = NFS4ERR_SERVERFAULT;
		T_EXIT0(nfs_status);
		goto out;
	}

	nfs_status = FSAL_encode_v4_multipath(da_addr_body, hosts_nmemb, hosts);
	if (nfs_status != NFS4_OK) {
		LogCrit(COMPONENT_PNFS, "FSAL_encode_v4_multipath failed, :%x",
			nfs_status);
		T_EXIT0(nfs_status);
		goto out;
	}

	T_EXIT0(nfs_status);
out:
	gsh_free(stripes);
	gsh_free(hosts);

	return nfs_status;
}

/**
 * @brief Get list of available devices
 *
 * We do not support listing devices and just set EOF without doing
 * anything.
 *
 * @param[in]     export_pub Export handle
 * @param[in]     type      Type of layout to get devices for
 * @param[in]     cb        Function taking device ID halves
 * @param[in,out] res       In/out and output arguments of the function
 *
 * @return Valid error codes in RFC 5661, pp. 365-6.
 */

static nfsstat4
kvsfs_getdevicelist(struct fsal_export *export_pub,
		     layouttype4 type,
		     void *opaque,
		     bool (*cb)(void *opaque,
			const uint64_t id),
		     struct fsal_getdevicelist_res *res)
{
	res->eof = true;
	return NFS4_OK;
}

void
export_ops_pnfs(struct export_ops *ops)
{
	ops->getdevicelist = kvsfs_getdevicelist;
	ops->fs_layouttypes = kvsfs_fs_layouttypes;
	ops->fs_layout_blocksize = kvsfs_fs_layout_blocksize;
	ops->fs_maximum_segments = kvsfs_fs_maximum_segments;
	ops->fs_loc_body_size = kvsfs_fs_loc_body_size;
}
void fsal_ops_pnfs(struct fsal_ops *ops)
{
	ops->getdeviceinfo = kvsfs_getdeviceinfo;
	ops->fs_da_addr_size = kvsfs_fs_da_addr_size;
}
/**
 * @brief Grant a layout segment.
 *
 * Grant a layout on a subset of a file requested.  As a special case,
 * lie and grant a whole-file layout if requested, because Linux will
 * ignore it otherwise.
 *
 * @param[in]     obj_pub  Public object handle
 * @param[in]     req_ctx  Request context
 * @param[out]    loc_body An XDR stream to which the FSAL must encode
 *                         the layout specific portion of the granted
 *                         layout segment.
 * @param[in]     arg      Input arguments of the function
 * @param[in,out] res      In/out and output arguments of the function
 *
 * @return Valid error codes in RFC 5661, pp. 366-7.
 */


static nfsstat4
kvsfs_layoutget(struct fsal_obj_handle *obj_hdl,
		 struct req_op_context *req_ctx,
		 XDR *loc_body,
		 const struct fsal_layoutget_arg *arg,
		 struct fsal_layoutget_res *res)
{
	struct kvsfs_fsal_module *kvsfs_fsal = NULL;
	struct kvsfs_fsal_obj_handle *myself;
	struct kvsfs_file_handle kvsfs_ds_handle;
	uint32_t stripe_unit = 0;
	nfl_util4 util = 0;
	struct pnfs_deviceid deviceid = DEVICE_ID_INIT_ZERO(FSAL_ID_EXPERIMENTAL);
	nfsstat4 nfs_status = 0;
	struct gsh_buffdesc ds_desc;
	kvsfs_fsal = (struct kvsfs_fsal_module *)
		container_of(obj_hdl->fsal, struct kvsfs_fsal_module, fsal);
	struct kvsfs_pnfs_mds_ctx *mds_ctx = kvsfs_fsal->mds_ctx;

	T_ENTER(">>> (%p, %p)", obj_hdl, arg);

	if (mds_ctx->pnfs_role < KVSFS_PNFS_MDS) {
		LogCrit(COMPONENT_PNFS,
			"MDS operation on Non-MDS export");
		T_EXIT0(mds_ctx->pnfs_role);
		return NFS4ERR_PNFS_NO_LAYOUT;
	}

	/* We support only SUPPORTED_LAYOUT_TYPE (i.e. LAYOUT4_NFSV4_1_FILES)
	 * layouts */

	if (arg->type != SUPPORTED_LAYOUT_TYPE) {
		LogCrit(COMPONENT_PNFS,
			"Unsupported layout type: %x",
			arg->type);
		T_EXIT0(arg->type);
		return NFS4ERR_UNKNOWN_LAYOUTTYPE;
	}

	/* Get basic information on the file and calculate the dimensions
	 *of the layout we can support. */

	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle,
			      obj_handle);

	/* TODO: permission check for res->segment.io_mode against to the
	 * current export and file handle
	 * This check should be performed against the open's permission that
	 * client requested earlier, and to implement it will need support
	 * for global open table.
	 */
	// Note: We give layout of the whole file
	res->segment.offset = 0;
	res->segment.length = NFS4_UINT64_MAX;

	/* TODO: Mark this file layout is locked so no further layoutget
	 * should be granted and error NFS4ERR_LAYOUTTRYLATER should be returned.
	 */
	res->return_on_close = true;

	/* We grant only one segment, and we want
	 * it back when the file is closed. */
	res->last_segment = true;

	// Back channel based notification is unavailable for now
	res->signal_available = false;

	stripe_unit = mds_ctx->mds->stripe_unit;
	 util |= stripe_unit | NFL4_UFLG_COMMIT_THRU_MDS;

	if ((util & NFL4_UFLG_STRIPE_UNIT_SIZE_MASK) != stripe_unit) {
		LogEvent(COMPONENT_PNFS,
			 "Invalid stripe_unit %u, truncated to %u",
			 stripe_unit, util);
		T_EXIT0(util);
		return NFS4ERR_SERVERFAULT;
	}

	deviceid.fsal_id = FSAL_ID_EXPERIMENTAL;
	LogDebug(COMPONENT_PNFS,
		 "devid nodeAddr %016"PRIx64,
		 deviceid.devid);

	memcpy(&kvsfs_ds_handle,
	       myself->handle, sizeof(struct kvsfs_file_handle));

	ds_desc.addr = &kvsfs_ds_handle;
	ds_desc.len = sizeof(struct kvsfs_file_handle);

	nfs_status = FSAL_encode_file_layout(
			loc_body,
			&deviceid,
			util,
			0,
			0,
			&req_ctx->ctx_export->export_id,
			1,
			&ds_desc);
	if (nfs_status) {
		LogCrit(COMPONENT_PNFS,
			"Failed to encode nfsv4_1_file_layout.");
		T_EXIT0(nfs_status);
		goto relinquish;
	}

	T_EXIT0(nfs_status);
relinquish:

	return nfs_status;
}

/**
 * @brief Potentially return one layout segment
 *
 * Since we don't make any reservations, in this version, or get any
 * pins to release, always succeed
 *
 * @param[in] obj_pub  Public object handle
 * @param[in] req_ctx  Request context
 * @param[in] lrf_body Nothing for us
 * @param[in] arg      Input arguments of the function
 *
 * @return Valid error codes in RFC 5661, p. 367.
 */

static nfsstat4
kvsfs_layoutreturn(struct fsal_obj_handle *obj_hdl,
		    struct req_op_context *req_ctx,
		    XDR *lrf_body,
		    const struct fsal_layoutreturn_arg *arg)
{
	struct kvsfs_fsal_obj_handle *myself;
	/* The private 'full' object handle */

	T_ENTER(">>> (%p, %p)", obj_hdl, arg);

	/* Sanity check on type */
	if (arg->lo_type != LAYOUT4_NFSV4_1_FILES) {
		LogCrit(COMPONENT_PNFS,
			"Unsupported layout type: %x",
			arg->lo_type);
		T_EXIT0(NFS4ERR_UNKNOWN_LAYOUTTYPE);
	return NFS4ERR_UNKNOWN_LAYOUTTYPE;
	}

	myself = container_of(obj_hdl,
			      struct kvsfs_fsal_obj_handle,
			      obj_handle);

	/* TODO: Unlock the layout lock from this file handle */

	T_EXIT0(NFS4_OK);
	return NFS4_OK;
}

/**
 * @brief Commit a segment of a layout
 *
 * This function is called once on every segment of a layout.  The
 * FSAL may avoid being called again after it has finished all tasks
 * necessary for the commit by setting res->commit_done to true.
 *
 * @param[in]     obj_hdl  Public object handle
 * @param[in]     req_ctx  Request context
 * @param[in]     lou_body An XDR stream containing the layout
 *                         type-specific portion of the LAYOUTCOMMIT
 *                         arguments.
 * @param[in]     arg      Input arguments of the function
 * @param[in,out] res      In/out and output arguments of the function
 *
 * @return Valid error codes in RFC 5661, p. 366.
 */

static nfsstat4
kvsfs_layoutcommit(struct fsal_obj_handle *obj_hdl,
		    struct req_op_context *req_ctx,
		    XDR *lou_body,
		    const struct fsal_layoutcommit_arg *arg,
		    struct fsal_layoutcommit_res *res)
{
	struct kvsfs_fsal_obj_handle *myself;
	/* The private 'full' object handle */
	// struct kvsfs_file_handle *kvsfs_handle;

	T_ENTER(">>> (%p, %p)", obj_hdl, arg);

	/* Sanity check on type */
	if (arg->type != LAYOUT4_NFSV4_1_FILES) {
		LogCrit(COMPONENT_PNFS,
			"Unsupported layout type: %x",
			arg->type);
		T_EXIT0(NFS4ERR_UNKNOWN_LAYOUTTYPE);
		return NFS4ERR_UNKNOWN_LAYOUTTYPE;
	}

	myself =
		container_of(obj_hdl,
			     struct kvsfs_fsal_obj_handle,
			     obj_handle);
	// kvsfs_handle = myself->handle;

	/** @todo: here, add code to actually commit the layout */
	res->size_supplied = false;
	res->commit_done = true;

	T_EXIT0(NFS4_OK);
	return NFS4_OK;
}


void
handle_ops_pnfs(struct fsal_obj_ops *ops)
{
	ops->layoutget = kvsfs_layoutget;
	ops->layoutreturn = kvsfs_layoutreturn;
	ops->layoutcommit = kvsfs_layoutcommit;
}

