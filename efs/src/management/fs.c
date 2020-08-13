/*
 * Filename: fs.c
 * Description: Echo controller.
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
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
 */

#include <stdio.h> /* sprintf */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <evhtp.h>
#include <json/json.h> /* for json_object */
#include <management.h>
#include <common/log.h>
#include <str.h>
#include <debug.h>
#include "internal/controller.h"
#include "internal/fs.h"

/**
 * ##############################################################
 * #		FS CREATE API'S					#
 * ##############################################################
 */
struct fs_create_api_req {
	const char *fs_name;
	const char *fs_options;
	/* ... */
};

struct fs_create_api_resp {
	/* ... */
};

struct fs_create_api {
	struct fs_create_api_req  req;
	struct fs_create_api_resp resp;
};

static int fs_create_send_response(struct controller_api *fs_create, void *args)
{
	int rc = 0;
	int resp_code = 0;
	struct request *request = NULL;

	request = fs_create->request;

	rc = request_get_errcode(request);
	if (rc != 0) {
		resp_code = errno_to_http_code(rc);
	} else {
		/* Resource got created. */
		resp_code = EVHTP_RES_CREATED;
	}

	log_debug("err_code : %d", resp_code);

	request_send_response(request, resp_code);

	return rc;
}

static int fs_create_process_data(struct controller_api *fs_create)
{
	int rc = 0;
	str256_t fs_name;
	int fs_name_len = 0;
	const char *fs_options = NULL;
	struct request *request = NULL;
	struct fs_create_api *fs_create_api = NULL;
	struct json_object *json_obj = NULL;
	struct json_object *json_fs_name_obj = NULL;
	struct json_object *json_fs_options_obj = NULL;

	request = fs_create->request;

	/**
	 * Process the fs_create data.
	 * 1. Parse JSON request data.
	 * 2. Compose efs_fs_create api params.
	 * 3. Send create fs request
	 * 4. Get response.
	 */
	rc = request_accept_data(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		fs_create_send_response(fs_create, NULL);
		goto error;
	}

	/* 1. Parse JSON request data. */
	fs_create_api = (struct fs_create_api*)fs_create->priv;

	json_obj = request_get_data(request);

	json_object_object_get_ex(json_obj,
				  "name",
				  &json_fs_name_obj);
	if (json_fs_name_obj == NULL) {
		log_err("No FS name.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_create_send_response(fs_create, NULL);
		goto error;
	}

	fs_create_api->req.fs_name = json_object_get_string(json_fs_name_obj);
	if (fs_create_api->req.fs_name == NULL) {
		log_err("No FS name.");
		request_set_errcode(request, EINVAL);
		fs_create_send_response(fs_create, NULL);
		goto error;
	}

	json_object_object_get_ex(json_obj,
				  "options",
				  &json_fs_options_obj);

	if (json_fs_options_obj != NULL) {
		fs_options =
		json_object_to_json_string_ext(json_fs_options_obj,
					       JSON_C_TO_STRING_SPACED);

		fs_create_api->req.fs_options = fs_options;
	}

	/* 2. Compose efs_fs_create api params. */
	fs_name_len = strlen(fs_create_api->req.fs_name);
	if (fs_name_len > 255) {
		log_err("FS name too long.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_create_send_response(fs_create, NULL);
		goto error;
	}

	log_debug("Creating FS : %s Options: %s.",
		  fs_create_api->req.fs_name,
		  fs_create_api->req.fs_options);

	str256_from_cstr(fs_name,
			 fs_create_api->req.fs_name,
			 fs_name_len);

	/* 3. Send create fs request */
	rc = efs_fs_create(&fs_name);
	request_set_errcode(request, -rc);
	log_debug("FS create status code : %d.", rc);

	request_next_action(fs_create);

error:
	return rc;
}

static int fs_create_process_request(struct controller_api *fs_create,
				     void *args)
{
	int rc = 0;
	struct request *request = NULL;

	request = fs_create->request;

	rc = request_validate_headers(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		fs_create_send_response(fs_create, NULL);
		goto error;
	}

	if (request_content_length(request) == 0) {
		/**
		 * Expecting create request fs info.
		 */
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_create_send_response(fs_create, NULL);
		goto error;
	}

	/* Set read data call back. */
	request_set_readcb(request, fs_create_process_data);

error:
	return rc;
}


static controller_api_action_func default_fs_create_actions[] =
{
	fs_create_process_request,
	fs_create_send_response,
};

static int fs_create_init(struct controller *controller,
		   struct request *request,
		   struct controller_api **api)
{
	int rc = 0;
	struct controller_api *fs_create = NULL;

	fs_create = malloc(sizeof(struct controller_api));
	if (fs_create == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	fs_create->request = request;
	fs_create->controller = controller;

	fs_create->name = "CREATE";
	fs_create->type = FS_CREATE_ID;
	fs_create->action_next = 0;
	fs_create->action_table = default_fs_create_actions;

	fs_create->priv = calloc(1, sizeof(struct fs_create_api));
	if (fs_create->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut parameter value. */
	*api = fs_create;
	fs_create = NULL;

error:
	if (fs_create) {
		if (fs_create->priv) {
			free(fs_create->priv);
		}

		free(fs_create);
	}

	return rc;
}

static void fs_create_fini(struct controller_api *fs_create)
{
	if (fs_create->priv) {
		free(fs_create->priv);
	}

	if (fs_create) {
		free(fs_create);
	}
}

/**
 * ##############################################################
 * #		FS DELETE API'S					#
 * ##############################################################
 */
struct fs_delete_api_req {
	const char * fs_name;
	/* ... */
};

struct fs_delete_api_resp {
	int status;
	/* ... */
};

struct fs_delete_api {
	struct fs_delete_api_req  req;
	struct fs_delete_api_resp resp;
};

static int fs_delete_send_response(struct controller_api *fs_delete, void *args)
{
	int rc = 0;
	int resp_code = 0;
	struct request *request = NULL;

	request = fs_delete->request;

	rc = request_get_errcode(request);
	if (rc != 0) {
		/* Send error response message. */
		resp_code = errno_to_http_code(rc);
	} else {
		resp_code = EVHTP_RES_200;
	}

	request_send_response(request, resp_code);
	return rc;
}

static int fs_delete_process_request(struct controller_api *fs_delete,
				     void *args)
{
	int rc = 0;
	str256_t fs_name;
	int fs_name_len = 0;
	struct request *request = NULL;
	struct fs_delete_api *fs_delete_api = NULL;

	request = fs_delete->request;

	rc = request_validate_headers(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		fs_delete_send_response(fs_delete, NULL);
		goto error;
	}

	if (request_content_length(request) != 0) {
		/**
		 * FS delete request doesn't expect any payload data.
		 */
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_delete_send_response(fs_delete, NULL);
		goto error;
	}

	/**
	 * Get the FS delete api info.
	 */
	fs_delete_api = (struct fs_delete_api*)fs_delete->priv;
	fs_delete_api->req.fs_name = request_api_file(request);
	if (fs_delete_api->req.fs_name == NULL) {
		log_err("No FS name.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_create_send_response(fs_delete, NULL);
		goto error;
	}

	fs_name_len = strlen(fs_delete_api->req.fs_name);
	if (fs_name_len > 255) {
		log_err("FS name too long.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_create_send_response(fs_delete, NULL);
		goto error;
	}

	/* Compose efs_fs_delete api params. */
	str256_from_cstr(fs_name,
			 fs_delete_api->req.fs_name,
			 fs_name_len);

	log_debug("Deleting FS : %s.", fs_delete_api->req.fs_name);

	/**
	 * Send fs delete request to the backend.
	 */
	rc = efs_fs_delete(&fs_name);
	request_set_errcode(request, -rc);

	log_debug("FS delete return code: %d.", rc);

	request_next_action(fs_delete);

error:
	return rc;
}

static controller_api_action_func default_fs_delete_actions[] =
{
	fs_delete_process_request,
	fs_delete_send_response,
};

static int fs_delete_init(struct controller *controller,
		  struct request *request,
		  struct controller_api **api)
{
	int rc = 0;
	struct controller_api *fs_delete = NULL;

	fs_delete = malloc(sizeof(struct controller_api));
	if (fs_delete == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	fs_delete->request = request;
	fs_delete->controller = controller;

	fs_delete->name = "DELETE";
	fs_delete->type = FS_DELETE_ID;
	fs_delete->action_next = 0;
	fs_delete->action_table = default_fs_delete_actions;

	fs_delete->priv = calloc(1, sizeof(struct fs_delete_api));
	if (fs_delete->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut Parameter value. */
	*api = fs_delete;
	fs_delete = NULL;

error:
	if (fs_delete) {
		if (fs_delete->priv) {
			free(fs_delete->priv);
		}

		free(fs_delete);
	}

	return rc;
}

static void fs_delete_fini(struct controller_api *fs_delete)
{
	if (fs_delete->priv) {
		free(fs_delete->priv);
	}

	if (fs_delete) {
		free(fs_delete);
	}
}

/**
 * ##############################################################
 * #		FS LIST API'S					#
 * ##############################################################
 */
struct fs_list_entry {
	char fs_name[256];
	char *fs_options;
	char *fs_endpoint_options;
	/* ... */
	LIST_ENTRY(fs_list_entry) entries;  /* Link. */
};

struct fs_list_api_req {
	const char *fs_name;
	/* ... */
};

struct fs_list_api_resp {
	LIST_HEAD(fs_list,
	fs_list_entry)		fs_list;
	/* ... */
};

struct fs_list_api {
	struct fs_list_api_req  req;
	struct fs_list_api_resp resp;
};

static int fs_list_send_response(struct controller_api *fs_list, void *args)
{
	int rc = 0;
	int resp_code = 0;
	struct fs_list_entry *fs_node = NULL;
	struct request *request = NULL;
	struct fs_list_api *fs_list_api = NULL;
	struct json_object *json_obj = NULL;
	struct json_object *json_resp_obj = NULL;
	struct json_object *json_fs_obj = NULL;

	request = fs_list->request;
	fs_list_api = (struct fs_list_api*)fs_list->priv;

	rc = request_get_errcode(request);
	if (rc != 0) {
		resp_code = errno_to_http_code(rc);
		goto out;
	}

	json_resp_obj = json_object_new_array();

	if (LIST_EMPTY(&fs_list_api->resp.fs_list)) {
		resp_code = EVHTP_RES_NOCONTENT;
		goto out;
	}

	LIST_FOREACH(fs_node, &fs_list_api->resp.fs_list, entries) {
		log_debug("FS entry:%s", fs_node->fs_name);

		/* Create json object */
		json_fs_obj = json_object_new_object();

		json_obj = json_object_new_string(fs_node->fs_name);
		json_object_object_add(json_fs_obj,
				       "fs-name",
				       json_obj);

		json_obj = json_tokener_parse(fs_node->fs_options);
		json_object_object_add(json_fs_obj,
				       "fs-options",
				       json_obj);

		json_obj = NULL;
		if (fs_node->fs_endpoint_options) {
			json_obj =
			json_tokener_parse(fs_node->fs_endpoint_options);
		}

		json_object_object_add(json_fs_obj,
				       "endpoint-options",
				       json_obj);

		json_object_array_add(json_resp_obj,
				json_fs_obj);
	}

	request_set_data(request, json_resp_obj);

	/* List Deletion. */
	while (!LIST_EMPTY(&fs_list_api->resp.fs_list)) {
		fs_node = LIST_FIRST(&fs_list_api->resp.fs_list);

		/* Remove the node entry from list. */
		LIST_REMOVE(fs_node, entries);

		/* Fre1e the dynamic node members. */
		free(fs_node->fs_endpoint_options);

		free(fs_node);
	}

	resp_code = EVHTP_RES_200;

out:
	log_debug("err_code : %d", resp_code);

	request_send_response(request, resp_code);
	return 0;
}

static int fs_list_add_entry(const struct efs_fs_list_entry *fs, void *args)
{
	int rc = 0;
	struct fs_list_entry *fs_node;
	struct fs_list_api *fs_list_api = NULL;

	fs_list_api = (struct fs_list_api *)args;

	fs_node = malloc(sizeof(struct fs_list_entry));
	if (fs_node == NULL) {
		rc = ENOMEM;
		log_err("No Mem : Fs list entry allocation failed.");
		goto error;
	}

	strcpy(fs_node->fs_name, fs->fs_name->s_str);

	fs_node->fs_options = "";

	if (fs->endpoint_info == NULL) {
		fs_node->fs_endpoint_options = NULL;
	} else {
		fs_node->fs_endpoint_options = strdup(fs->endpoint_info);
		if (fs_node->fs_endpoint_options == NULL ) {
			rc = -ENOMEM;
			goto error;
		}
	}

	log_debug("FS scan cb : fs entry : name = %s, endpoint_options = %s",
		  fs_node->fs_name, fs_node->fs_endpoint_options);

	LIST_INSERT_HEAD(&fs_list_api->resp.fs_list, fs_node, entries);

error:
	return rc;
}

static int fs_list_process_request(struct controller_api *fs_list, void *args)
{
	int rc = 0;
	str256_t fs_name;
	int fs_name_len = 0;
	struct request *request = NULL;
	struct fs_list_api *fs_list_api = NULL;

	request = fs_list->request;

	rc = request_validate_headers(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		fs_list_send_response(fs_list, NULL);
		goto error;
	}

	if (request_content_length(request) != 0) {
		/**
		 * FS delete request doesn't expect any payload data.
		 */
		rc = EINVAL;
		request_set_errcode(request, rc);
		fs_list_send_response(fs_list, NULL);
		goto error;
	}

	/**
	 * Get the FS list api info.
	 */
	fs_list_api = (struct fs_list_api*)fs_list->priv;
	fs_list_api->req.fs_name = request_api_file(request);

	if (fs_list_api->req.fs_name) {
		/* Compose efs_fs_list api params. */
		log_debug("Getting FS : %s. info", fs_list_api->req.fs_name);

		fs_name_len = strlen(fs_list_api->req.fs_name);
		str256_from_cstr(fs_name,
			 	 fs_list_api->req.fs_name,
			 	 fs_name_len);
	}

	/**
	 * Send fs delete request to the backend.
	 */

	LIST_INIT(&(fs_list_api->resp.fs_list));

	log_debug("Getting FS list.");

	efs_fs_scan_list(fs_list_add_entry, fs_list_api);

	log_debug("FS list return code: %d.", rc);

	request_next_action(fs_list);

error:
	return rc;
}

static controller_api_action_func default_fs_list_actions[] =
{
	fs_list_process_request,
	fs_list_send_response,
};

static int fs_list_init(struct controller *controller,
		   struct request *request,
		   struct controller_api **api)
{
	int rc = 0;
	struct controller_api *fs_list = NULL;

	fs_list = malloc(sizeof(struct controller_api));
	if (fs_list == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	fs_list->request = request;
	fs_list->controller = controller;

	fs_list->name = "LIST";
	fs_list->type = FS_CREATE_ID;
	fs_list->action_next = 0;
	fs_list->action_table = default_fs_list_actions;

	fs_list->priv = calloc(1, sizeof(struct fs_list_api));
	if (fs_list->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut parameter value. */
	*api = fs_list;
	fs_list = NULL;

error:
	if (fs_list) {
		if (fs_list->priv) {
			free(fs_list->priv);
		}

		free(fs_list);
	}

	return rc;
}

static void fs_list_fini(struct controller_api *fs_list)
{
	if (fs_list->priv) {
		free(fs_list->priv);
	}

	if (fs_list) {
		free(fs_list);
	}
}

/**
 * ##############################################################
 * #		FS CONTROLLER API'S				#
 * ##############################################################
 */
#define FS_NAME	"fs"
#define FS_API_URI	"/fs"

static char *default_fs_api_list[] =
{
#define XX(uc, lc, _)	#lc,
	FS_API_MAP(XX)
#undef XX
};

static struct controller_api_table fs_api_table [] =
{
#define XX(uc, lc, method)	{ #lc, #method, FS_ ## uc ## _ID },
	FS_API_MAP(XX)
#undef XX
};

static int fs_api_name_to_id(char *api_name, enum fs_api_id *api_id)
{
	int rc = EINVAL;
	int idx = 0;

	for (idx = 0; idx < FS_API_COUNT; idx++) {
		if (!strcmp(fs_api_table[idx].method, api_name)) {
			*api_id = fs_api_table[idx].id;
			rc = 0;
			break;
		}
	}

	return rc;
}

static int fs_api_init(char *api_name,
	       struct controller *controller,
	       struct request *request,
	       struct controller_api **api)
{
	int rc = 0;
	enum fs_api_id api_id;
	struct controller_api *fs_api = NULL;

	rc = fs_api_name_to_id(api_name, &api_id);
	if (rc != 0) {
		log_err("Unknown fs api : %s.\n", api_name);
		goto error;
	}

	switch(api_id) {
#define XX(uc, lc, _)							\
	case FS_ ## uc ## _ID:						\
		rc = fs_ ## lc ## _init(controller, request, &fs_api);	\
		break;
		FS_API_MAP(XX)
#undef XX
	default:
		log_err("Not supported api : %s", api_name);
	}

	/* Assign the InOut variable api value. */
	*api = fs_api;

error:
	return rc;
}

static void fs_api_fini(struct controller_api *fs_api)
{
	char *api_name = NULL;
	enum fs_api_id api_id;

	api_name = fs_api->name;
	api_id = fs_api->type;

	switch(api_id) {
#define XX(uc, lc, _)							\
	case FS_ ## uc ## _ID:						\
		fs_ ## lc ## _fini(fs_api);				\
		break;
		FS_API_MAP(XX)
#undef XX
	default:
		log_err("Not supported api : %s", api_name);
	}
}

static struct controller default_fs_controller =
{
	.name	  = FS_NAME,
	.type	  = CONTROLLER_FS_ID,
	.api_uri  = FS_API_URI,
	.api_list = default_fs_api_list,
	.api_init = fs_api_init,
	.api_fini = fs_api_fini,
};

int ctl_fs_init(struct server *server, struct controller **controller)
{
	int rc = 0;

	struct controller *fs_controller = NULL;

	fs_controller = malloc(sizeof(struct controller));
	if (fs_controller == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Init fs_controller. */
	*fs_controller = default_fs_controller;
	fs_controller->server = server;

	/* Assign the return valure. */
	*controller = fs_controller;

error:
	return rc;
}

void ctl_fs_fini(struct controller *fs_controller)
{
	free(fs_controller);
	fs_controller = NULL;
}
