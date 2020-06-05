/*
 * Filename: config_impl.h
 * Description: helper functions API to update ganehsa config.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
 *
 */

#ifndef _CONFIG_IMPL_H
#define _CONFIG_IMPL_H

#include <stdint.h>
#include <json/json.h> /* for json_object */
#include "gsh_list.h"
#include "str.h"

#define PULGINS_DIR                             "/usr/lib64/ganesha/"
#define FSAL_NAME                               "KVSFS"
#define FSAL_SHARED_LIBRARY                     "/usr/lib64/ganesha/libfsalkvsfs.so.4.2.0"
#define EFS_CONFIG                              "/etc/efs/efs.conf"
#define DOMAIN_NAME				"localdomain"
#define SERIALIZE_BUFFER_DEFAULT_SIZE           2048

struct export_fsal_block {
	str256_t name;
	str256_t efs_config;
};

/* Default config block */
struct kvsfs_block {
	str256_t fsal_shared_library;
};

struct fsal_block {
	struct kvsfs_block kvsfs_block;
};

/* nfs_core_param */
struct nfs_core_param_block {
	uint32_t nb_worker;
	uint32_t manage_gids_expiration;
	str256_t plugins_dir;
};

/* nfsv4_block */
struct nfsv4_block {
	str256_t domainname;
	bool graceless;
};

/* client_block */
struct client_block {
	/* TODO have glist here for more then one client block in single
	 * export
	 * struct glist_head glist;
	 */
	str256_t clients;
	str256_t squash;
	str256_t access_type;
	str256_t protocols;
};

/* export_block */
struct export_block {
	struct glist_head glist;
	uint16_t export_id;
	str256_t path;
	str256_t pseudo;
	struct export_fsal_block fsal_block;
	str256_t sectype;
	str256_t filesystem_id;
	struct client_block client_block;

};

/* ganesha config */
struct ganesha_config {
	struct fsal_block fsal_block;
	struct nfsv4_block nfsv4_block;
	struct nfs_core_param_block core_parm;
	struct export_block export_block;
};

/* Gets default block values. */
void get_default_block(struct ganesha_config *config);

/*creates export_block structure from json */
int json_to_export_block(const char *name, uint16_t id, struct json_object *obj,
			 struct export_block *block);

/* Serializes ganesha_config struture and dump into config file. */
int ganesha_config_dump(struct ganesha_config *config);
#endif /* _CONFIG_IMPL_H */
