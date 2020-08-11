/*
 * Filename: config_impl.c.
 * Description: helper functions API for creating/deleting ganesha config.
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

#include "abstract_mem.h"
#include "config_impl.h"

#define GANEHSA_CONF		"/etc/ganesha/ganesha.conf"

/* serialized buffer container */
struct serialized_buffer{
	void *data; /* data container */
	size_t capacity; /* capacity of the buffer */
	int length; /* lenth of the buffer */
};

/* Initialize the serail buffer. 
 *
 * @param[in]: buffer - buffer pointer to be initailze.
 *
 * return void.
 */
static void init_serialized_buffer(struct serialized_buffer **buffer)
{
	struct serialized_buffer *buff = NULL;
	buff = gsh_calloc(1, sizeof(struct serialized_buffer));
	buff->data = gsh_calloc(1, SERIALIZE_BUFFER_DEFAULT_SIZE);
	buff->capacity = SERIALIZE_BUFFER_DEFAULT_SIZE;
	buff->length = 0;

	*buffer = buff;
	return;
}

/* Finalize the serail buffer 
 * 
 * @prama[in]: buffer - ptr to container.
 *
 * return void.
 */
static void free_serialize_buffer(struct serialized_buffer *buffer)
{
	gsh_free(buffer->data);
	gsh_free(buffer);
}

/* expand the buffer capacity 
 *
 * @param[in/out]: buffer - container pointer.
 * @param[in]: new capacity of container.
 *
 * @return void.
 */
static void reserve(struct serialized_buffer *buffer, int capacity)
{
	buffer->data = realloc(buffer->data, capacity);
	assert(buffer->data != NULL);
}

/* append data into buffer.
 * 
 * @param[out]; buffer - data container.
 * @param[in]: data - data source.
 * @paramo[in]: len - number of bytes to copy from data to buffer.
 *
 * return void.
 */
static void append_data(struct serialized_buffer *buffer, char *data, int len)
{
	if (buffer->capacity < buffer->length + len) {
		reserve(buffer, buffer->capacity * 2);
	}

	assert(buffer->capacity >= buffer->length + len);
	memcpy((char *)buffer->data + buffer->length, data, len);
	buffer->length += len;
	return;
}

/*creates export_fsal_block structure from json */
void json_to_export_fsal_block(struct json_object *obj,
			       struct export_fsal_block *block)
{
	str256_from_cstr(block->name, FSAL_NAME, strlen(FSAL_NAME));
	str256_from_cstr(block->efs_config, EFS_CONFIG, strlen(EFS_CONFIG));
	return;
}

/*creates client_block structure from json */
int json_to_client_block(struct json_object *obj,
			 struct client_block *block)
{
	int rc = 0;
	struct json_object *json_obj = NULL;
	const char *str = NULL;

	json_object_object_get_ex(obj, "clients", &json_obj);
	str = json_object_get_string(json_obj);
	if (str == NULL) {
		rc = -EINVAL;
		goto out;
	}
	str256_from_cstr(block->clients, str, strlen(str));

	str = NULL;
	json_object_object_get_ex(obj, "Squash", &json_obj);
	str = json_object_get_string(json_obj);
	if (str == NULL) {
		rc = -EINVAL;
		goto out;
	}
	str256_from_cstr(block->squash, str, strlen(str));

	str = NULL;
	json_object_object_get_ex(obj, "access_type", &json_obj);
	str = json_object_get_string(json_obj);
	if (str == NULL) {
		rc = -EINVAL;
		goto out;
	}
	str256_from_cstr(block->access_type, str, strlen(str));

	str = NULL;
	json_object_object_get_ex(obj, "protocols", &json_obj);
	str = json_object_get_string(json_obj);
	if (str == NULL) {
		rc = -EINVAL;
		goto out;
	}
	str256_from_cstr(block->protocols, str, strlen(str));
out:
	return rc;
}

/*creates export_block structure from json */
int json_to_export_block(const char *name, uint16_t id, struct json_object *obj,
			 struct export_block *block)
{
	int rc = 0;
	struct json_object *json_obj = NULL;
	const char *str = NULL;

	block->export_id = id;
	str256_from_cstr(block->path, name, strlen(name));
	str256_from_cstr(block->pseudo, name, strlen(name));

	json_to_export_fsal_block(obj, &block->fsal_block);
	json_object_object_get_ex(obj, "secType", &json_obj);
	str = json_object_get_string(json_obj);
	if (str == NULL) {
		rc = -EINVAL;
		goto out;
	}
	str256_from_cstr(block->sectype, str, strlen(str));
	str = NULL;

	json_object_object_get_ex(obj, "Filesystem_id", &json_obj);
	str = json_object_get_string(json_obj);
	if (str == NULL) {
		rc = -EINVAL;
		goto out;
	}
	str256_from_cstr(block->filesystem_id, str, strlen(str));
	str = NULL;
	rc = json_to_client_block(obj, &block->client_block);
out:
	return rc;
}

/* creates kvsfs_block structure */
static void get_kvsfs_block(struct kvsfs_block *block)
{
	str256_from_cstr(block->fsal_shared_library, FSAL_SHARED_LIBRARY,
			strlen(FSAL_SHARED_LIBRARY));
}

/* creates fsal_block structure */
static void get_fsal_block(struct fsal_block *block)
{
	get_kvsfs_block(&block->kvsfs_block);
}

/* creates nfs_core_param structure */
static void get_nfs_core_param_block(struct nfs_core_param_block *block)
{
	block->nb_worker = 1;
	block->manage_gids_expiration = 3600;
	str256_from_cstr(block->plugins_dir, PULGINS_DIR, strlen(PULGINS_DIR));
}

/* creates nfsv4 structure */
static void get_nfsv4_block(struct nfsv4_block *block)
{
	str256_from_cstr(block->domainname, DOMAIN_NAME, strlen(DOMAIN_NAME));
	block->graceless = true;
}

/* serializes fsal_block structure to bufferer */
static void fsal_block_to_buffer(struct fsal_block *block,
				 struct serialized_buffer *buffer)
{
	char str[256];
	sprintf(str, "FSAL {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tCORTX-FS {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\tFSAL_Shared_Library = %s;\n",
		block->kvsfs_block.fsal_shared_library.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));
}

/* serializes nfs_core_param structure to buffer */
static void nfs_core_param_to_buffer(struct nfs_core_param_block *block,
				     struct serialized_buffer *buffer)
{
	char str[256];

	sprintf(str, "NFS_Core_Param {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tNb_Worker = %u;\n", block->nb_worker);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tManage_Gids_Expiration = %u;\n",
			block->manage_gids_expiration);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tPlugins_Dir = %s;\n", PULGINS_DIR);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));
}

/* serializes nfsv4_block structure to buffer */
static void nfsv4_to_buffer(struct nfsv4_block *block,
			    struct serialized_buffer *buffer)
{
	char str[256];

	sprintf(str, "NFSv4 {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tDomainName = %s;\n", DOMAIN_NAME);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tGraceless = %s;\n", "YES");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));
}

/* serializes client_block structure to buffer */
static void client_to_buffer(struct client_block *block,
			     struct serialized_buffer *buffer)
{
	char str[256];
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tclient {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\tclients = %s;\n", block->clients.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\tSquash = %s;\n", block->squash.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\taccess_type = %s;\n",  block->access_type.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\tprotocols = %s;\n", block->protocols.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));
}

/* serializes export_block structure to buffer */
static void export_to_buffer(struct export_block *block,
			     struct serialized_buffer *buffer)
{
	char str[256];

	memset(str, '\0', sizeof(str));
	sprintf(str, "EXPORT {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tExport_Id =  %d;\n", block->export_id);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tPath = %s;\n", block->path.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tPseudo = /%s;\n", block->pseudo.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tFSAL {\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\tNAME = %s;\n", block->fsal_block.name.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t\tcortxfs_config = %s;\n",
		block->fsal_block.efs_config.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\t}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tSecType = %s;\n", block->sectype.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	sprintf(str, "\tFilesystem_id = %s;\n", block->filesystem_id.s_str);
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));

	client_to_buffer(&block->client_block, buffer);

	sprintf(str, "}\n");
	append_data(buffer, str, strlen(str));
	memset(str, '\0', sizeof(str));
}

/* serializes ganesha_config structure to buffer */
static void ganesha_config_to_buffer(struct ganesha_config *config,
				     struct serialized_buffer *buffer)
{
	fsal_block_to_buffer(&config->fsal_block, buffer);
	nfs_core_param_to_buffer(&config->core_parm, buffer);
	nfsv4_to_buffer(&config->nfsv4_block, buffer);
}

void get_default_block(struct ganesha_config *config)
{
	get_fsal_block(&config->fsal_block);
	get_nfs_core_param_block(&config->core_parm);
	get_nfsv4_block(&config->nfsv4_block);
}

int ganesha_config_dump(struct ganesha_config *config)
{
	int rc = 0;
	struct serialized_buffer *buff;
	char *tmp_config_path = "/etc/ganesha/tmpganesha";
	config_file_t config_struct = NULL;
	struct config_error_type err_type;

	init_serialized_buffer(&buff);
	ganesha_config_to_buffer(config, buff);

	struct export_block *entry;
	struct glist_head *glist;
	struct glist_head *head = &config->export_block.glist;

	glist_for_each(glist, head) {
		entry = glist_entry(glist, struct export_block, glist);
		export_to_buffer(entry, buff);
	}
	/* open tmp file and dump buffer */
	FILE *filep = fopen(tmp_config_path, "w");
	fwrite(buff->data, buff->length, 1, filep);
	fclose(filep);

	/* Create a memstream for parser+processing error messages */
	if (!init_error_type(&err_type)) {
		rc = -1;
		goto out;
	}

	config_struct = config_ParseFile(tmp_config_path, &err_type);
	if (!config_error_no_error(&err_type)) {
		LogCrit(COMPONENT_CONFIG,
			"Error while parsing new configuration file %s",
			tmp_config_path);
		report_config_errors(&err_type, NULL, config_errs_to_log);
		rc = -1;
		goto out;
	}
	/* If everything is correct, rename tmp file to ganesha.conf */
	rc = rename(tmp_config_path, GANEHSA_CONF);
	if (rc < 0) {
		LogCrit(COMPONENT_CONFIG, "Cannot rename to %s", GANEHSA_CONF);
	}
out:
	config_Free(config_struct);
	free_serialize_buffer(buff);
	return rc;
}
