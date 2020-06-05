/*
 * Filename: config.c
 * Description: ganesha config functions API.
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

#include "log.h"
#include "config_parsing.h"
#include "config_impl.h"
#include "kvsfs_methods.h"
#include "efs.h"

struct ganesha_config *ganesha_config;

/* Adds export block with Export_id=ep_id in ganesha config.
 *
 * @param[in]: ep_name - endpoint name.
 * @param[in]: ep_id - endpoint id.
 * @param[in]: ef-info - endpoint information.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsfs_add_export(const char *ep_name, uint16_t ep_id, const char *ep_info)
{
	int rc = 0;
	struct json_object *obj = NULL;
	struct export_block *export_block = NULL;

	obj = json_tokener_parse(ep_info);
	if (obj == NULL) {
		rc = -EINVAL;
		goto out;
	}
	export_block = gsh_calloc(1, sizeof(struct export_block));

	/* json to export block structure */
	rc = json_to_export_block(ep_name, ep_id, obj, export_block);
	if (rc < 0) {
		gsh_free(export_block);
		LogMajor(COMPONENT_FSAL, "json object is corrupt");
		goto out;
	}
	/* add export block to list */
	glist_add_tail(&ganesha_config->export_block.glist,
				   &export_block->glist);

	/* dump ganesha_config structre to file */
	rc = ganesha_config_dump(ganesha_config);
	if (rc < 0) {
		/* remove last corrupted entry form list */
		glist_del(&export_block->glist);
		gsh_free(export_block);
		LogMajor(COMPONENT_FSAL, "Can't dump list as config file");
	}
out:
	/* free json object */
	if (obj) {
		json_object_put(obj);
	}
	return rc;
}

/* Scans the each endpoint stroed in backend and save in memory list.
 *
 * @param[in]: ep -  endpoint information structure.
 * @param[in]: cb_ctx - context.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
static int endpoint_scan_cb(const struct efs_endpoint_info *ep, void *cb_ctx)
{
	int rc = 0;
	struct export_block *export_block = NULL;
	struct json_object *obj = NULL;

	export_block = gsh_calloc(1, sizeof(struct export_block));
	/* string to json object */
	obj = json_tokener_parse(ep->ep_info);
	if (obj == NULL) {
		rc = -EINVAL;
		goto out;
	}
	/* JSON to export structure */
	rc = json_to_export_block(ep->ep_name->s_str, ep->ep_id, obj, export_block);
	if (rc < 0) {
		LogMajor(COMPONENT_FSAL, "json object is corrupt");
		gsh_free(export_block);
		goto out;
	}
	/* add export block found during scan to list */
	glist_add(&ganesha_config->export_block.glist, &export_block->glist);
out:
	/* free json object */
	if (obj) {
		json_object_put(obj);
	}
	return rc;
}

/* Initialize ganesha config and fill default options
 *
 * @param - void.
 *
 * @retrun - 0 on success else abort.
 */
static int ganesha_config_init(void)
{
	ganesha_config = gsh_calloc(1, sizeof(struct ganesha_config));
	/* intialize list */
	glist_init(&ganesha_config->export_block.glist);
	/* get default block */
	get_default_block(ganesha_config);
	return 0;
}

/* finalize the ganesha config
 *
 * @param - void.
 *
 * @return - void.
 */
static void ganesha_config_fini(void)
{
	struct glist_head *iter = NULL;
	struct glist_head *next = NULL;

	if (glist_empty(&ganesha_config->export_block.glist)) {
		gsh_free(ganesha_config);
		return;
	}

	glist_for_each_safe(iter, next, &ganesha_config->export_block.glist) {
	struct export_block *entry = glist_entry(iter, struct export_block,
						 glist);
			glist_del(&entry->glist);
			gsh_free(entry);
	}
	gsh_free(ganesha_config);
}

/* Called during initialization of efs, if endpoint/s present in backend then
 * prepare in memory list and dump this list to create new ganesha config.
 *
 * @param void.
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */
int kvsfs_config_init(void)
{
	int rc = 0;

	assert(ganesha_config == NULL);
	rc = ganesha_config_init();

	/* get endpoint stroed in backend storage */
	rc = efs_endpoint_scan(endpoint_scan_cb, NULL);
	if (rc < 0 ) {
		LogMajor(COMPONENT_FSAL, "efs_endpoint_scan failed");
		goto out;
	}
	/* dump structure to config_file */
	rc = ganesha_config_dump(ganesha_config);
	if (rc < 0) {
		LogMajor(COMPONENT_FSAL, "Can't dump list as config file");
		goto out;
	}
	return rc;
out:
	if (ganesha_config) {
		ganesha_config_fini();
		ganesha_config = NULL;
	}
	return rc;
}

/* Removed export block, Export_id=ep_id from ganesha config
 *
 * @parma[in]: ep_id endpint_id(export_id)
 *
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int kvsfs_remove_export(uint16_t ep_id)
{
	int rc = -ENOENT;

	struct glist_head *iter = NULL;
	struct glist_head *next = NULL;

	if (glist_empty(&ganesha_config->export_block.glist)) {
		LogMajor(COMPONENT_FSAL, "No endpoint available");
		goto out;
	}

	glist_for_each_safe(iter, next, &ganesha_config->export_block.glist) {
	struct export_block *entry = glist_entry(iter, struct export_block,
						 glist);
		/* if endpoint found, remove fromlist */
		if (entry->export_id == ep_id) {
			rc = 0;
			glist_del(&entry->glist);
			gsh_free(entry);
			/* dump structure to config_file */
			rc = ganesha_config_dump(ganesha_config);
			if (rc < 0) {
				LogMajor(COMPONENT_FSAL, "Can't dump list as config file");
			}
			break;
		}
	}

	if (rc == -ENOENT) {
		LogMajor(COMPONENT_FSAL, "Can't find endpoint with id = %d",
				ep_id);
	}
out:
	return rc;
}

/* Removes in-memeory list.
 *
 * @param: void.
 *
 * @return 0 .
 */
int kvsfs_config_fini(void)
{
	if (ganesha_config) {
		ganesha_config_fini();
		ganesha_config = NULL;
	}
	return 0;
}
