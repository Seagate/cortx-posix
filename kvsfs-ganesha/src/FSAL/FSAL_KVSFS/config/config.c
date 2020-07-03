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

#include <regex.h>
#include "log.h"
#include "config_parsing.h"
#include "config_impl.h"
#include "kvsfs_methods.h"
#include "efs.h"

struct ganesha_config *ganesha_config;

/*
 * The NFS Ganesha specific keywords are taken from
 * nfs-ganesha-eos/src/config_samples/config.txt
 * These rules might need update in future.
 * Note:
 * struct export_block  might also need update in future.
 * TODO:
 * Currently multiple client blocks per exports are not there.
 * Jatinder confirms this is needed in future. Need a ticket
 * to track it.
 */

#define FILESYSTEM_MAJOR_MINOR_REGEX "^[0-9]+\\.[0-9]+$"

const char *SecType_keywords[] = {"none", "sys", "krb5", "krb5i", "krb5p"};
const char *Squash_keywords[] = {
		"root", "root_squash", "rootsquash", "rootid", "root_id_squash",
		"rootidsquash", "all", "all_squash", "allsquash", "all_anomnymous",
		"allanonymous", "no_root_squash", "none", "noidsquash"};
const char *Access_Type_keywords[] = {"None", "RW", "RO", "MDONLY", "MDONLY_RO"};
const char *Protocols_keywords[] = {"4", "NFS4", "V4", "NFSv4"};

static int regex_match(const char *str, const char *pattern)
{
	regex_t preg;
	int rc = 0;
	size_t nmatch = 2;
	regmatch_t pmatch[2];

	rc = regcomp(&preg, pattern, REG_EXTENDED | REG_ICASE);
	if (rc != 0) {
		LogMajor(COMPONENT_FSAL, "regcomp() failed: %d", rc);
		goto out;
	}

	rc = regexec(&preg, str, nmatch, pmatch, 0);
	if (rc != 0) {
		LogMajor(COMPONENT_FSAL, "regexec() failed: %d, %s, %s",
			 rc, str, pattern);
		goto out;
	}
out:
	regfree(&preg);
	return rc;
}

static int validate_export_block(const struct export_block *block)
{
	int rc = 0;
	int idx = 0;
	int array_nmemb = 0;

	if (block == NULL) {
		LogMajor(COMPONENT_FSAL, "block NULL");
		rc = -EINVAL;
		goto out;
	}

	if ((block->path.s_len == 0) || (block->path.s_str[0] == '\0')) {
		LogMajor(COMPONENT_FSAL, "block->path empty");
		rc = -EINVAL;
		goto out;
	}

	if ((block->pseudo.s_len == 0) || (block->pseudo.s_str[0] == '\0')) {
		LogMajor(COMPONENT_FSAL, "block->pseudo empty");
		rc = -EINVAL;
		goto out;
	}

	if ((block->fsal_block.name.s_len == 0) ||
	   (block->fsal_block.name.s_str[0] == '\0')) {
		LogMajor(COMPONENT_FSAL, "block->fsal_block.name empty");
		rc = -EINVAL;
		goto out;
	}

	if ((block->fsal_block.efs_config.s_len == 0) ||
	    (block->fsal_block.efs_config.s_str[0] == '\0')) {
		LogMajor(COMPONENT_FSAL, "block->fsal_block.efs_config empty");
		rc = -EINVAL;
		goto out;
	}

	array_nmemb = sizeof(SecType_keywords)/sizeof(SecType_keywords[0]);
	for (idx = 0; idx < array_nmemb; idx++) {
		if (!strcmp(block->sectype.s_str, SecType_keywords[idx]) &&
			    block->sectype.s_len ==
			    strlen(SecType_keywords[idx])) {
			break;
		}
	}

	if (idx == array_nmemb) {
		LogMajor(COMPONENT_FSAL, "block->sectype invalid, %s",
			 block->sectype.s_str);
		rc = -EINVAL;
		goto out;
	}

	if ((block->filesystem_id.s_len == 0) ||
	    (block->filesystem_id.s_str[0] == '\0')) {
		LogMajor(COMPONENT_FSAL, "block->filesystem_id empty");
		rc = -EINVAL;
		goto out;
	}

	if (regex_match(block->filesystem_id.s_str,
		FILESYSTEM_MAJOR_MINOR_REGEX)) {
		 LogMajor(COMPONENT_FSAL,
			 "filesystem_id major.minor format err: %s",
			 block->filesystem_id.s_str);
		rc = -EINVAL;
		goto out;
	}

	array_nmemb = sizeof(Squash_keywords)/sizeof(Squash_keywords[0]);
	for (idx = 0; idx < array_nmemb; idx++) {
		if (!strcmp(block->client_block.squash.s_str,
		    Squash_keywords[idx]) && block->client_block.squash.s_len ==
		    strlen(Squash_keywords[idx])) {
			break;
		}
	}

	if (idx == array_nmemb) {
		LogMajor(COMPONENT_FSAL,
			 "block->client_block.squash invalid, %s",
			 block->client_block.squash.s_str);
		rc = -EINVAL;
		goto out;
	}

	array_nmemb = sizeof(Access_Type_keywords)/
		      sizeof(Access_Type_keywords[0]);
	for (idx = 0; idx < array_nmemb; idx++) {
		if (!strcmp(block->client_block.access_type.s_str,
		    Access_Type_keywords[idx]) &&
		    block->client_block.access_type.s_len ==
		    strlen(Access_Type_keywords[idx])) {
			break;
		}
	}

	if (idx == array_nmemb) {
		LogMajor(COMPONENT_FSAL,
			 "block->client_block.access_typeinvalid, %s",
			 block->client_block.access_type.s_str);
		rc = -EINVAL;
		goto out;
	}

	array_nmemb = sizeof(Protocols_keywords)/sizeof(Protocols_keywords[0]);
	for (idx = 0; idx < array_nmemb; idx++) {
		if (!strcmp(block->client_block.protocols.s_str,
		    Protocols_keywords[idx]) &&
		    block->client_block.protocols.s_len ==
		    strlen(Protocols_keywords[idx])) {
			break;
		}
	}

	if (idx == array_nmemb) {
		LogMajor(COMPONENT_FSAL, "block->client_block.protocol, %s",
			 block->client_block.protocols.s_str);
		rc = -EINVAL;
		goto out;
	}

out:
	return rc;
}

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

	/* validate this export block */
	rc = validate_export_block(export_block);
	if (rc < 0) {
		gsh_free(export_block);
		LogMajor(COMPONENT_FSAL,
			 "export block object has invalid params");
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

	/* validate this export block */
	rc = validate_export_block(export_block);
	if (rc < 0) {
		LogMajor(COMPONENT_FSAL,
			 "export block object has invalid params");
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
