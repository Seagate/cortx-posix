/*
 * Filename: endpoint.c
 * Description: Export controller.
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
 * #		ENDPOINT CREATE API'S				#
 * ##############################################################
 */

struct endpoint_create_api_req {
	const char *endpoint_name;
	const char *endpoint_options;
	/* ... */
};

struct endpoint_create_api_resp {
	/* ... */
};

struct endpoint_create_api {
	struct endpoint_create_api_req  req;
	struct endpoint_create_api_resp resp;
};

static int endpoint_create_send_response(struct controller_api *endpoint_create,
				       void *args)
{
	int rc = 0;
	int resp_code = 0;
	struct request *request = NULL;

	request = endpoint_create->request;

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

static int endpoint_create_process_data(struct controller_api *endpoint_create)
{
	int rc = 0;
	str256_t endpoint_name;
	int endpoint_name_len = 0;
	struct request *request = NULL;
	struct endpoint_create_api *endpoint_create_api = NULL;
	struct json_object *json_obj = NULL;
	struct json_object *json_fs_name_obj = NULL;
	struct json_object *json_endpoint_options_obj = NULL;

	request = endpoint_create->request;

	/**
	 * Process the endpoint_create data.
	 * 1. Parse JSON request data.
	 * 2. Compose eendpoint_endpoint_create api params.
	 * 3. Send create endpoint request
	 * 4. Get response.
	 */
	rc = request_accept_data(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	/* 1. Parse JSON requst data. */
	endpoint_create_api = (struct endpoint_create_api*)endpoint_create->priv;

	json_obj = request_get_data(request);
	json_object_object_get_ex(json_obj,
				  "name",
				  &json_fs_name_obj);

	if (json_fs_name_obj == NULL) {
		log_err("No FS name.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	endpoint_create_api->req.endpoint_name =
		json_object_get_string(json_fs_name_obj);
	if (endpoint_create_api->req.endpoint_name == NULL) {
		log_err("No FS name.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	json_object_object_get_ex(json_obj,
				  "options",
				  &json_endpoint_options_obj);

	if (json_endpoint_options_obj == NULL) {
		log_err("No endpoint options.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	/* 2. Compose eendpoint_endpoint_create api params. */
	endpoint_name_len = strlen(endpoint_create_api->req.endpoint_name);
	if (endpoint_name_len > 255) {
		log_err("FS name too long.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	str256_from_cstr(endpoint_name,
			 endpoint_create_api->req.endpoint_name,
			 endpoint_name_len);

	/* endpoint options */
	endpoint_create_api->req.endpoint_options =
	json_object_to_json_string(json_endpoint_options_obj);

	if (endpoint_create_api->req.endpoint_options == NULL) {
		log_err("Invalid endpoint options.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	log_debug("Creating ENDPOINT : %s, options : %s.",
		  endpoint_create_api->req.endpoint_name,
		  endpoint_create_api->req.endpoint_options);

	/* 3. Send create endpoint request */
	rc = cfs_endpoint_create(&endpoint_name,
				 endpoint_create_api->req.endpoint_options);
	request_set_errcode(request, -rc);
	log_debug("ENDPOINT create status code : %d.", rc);

	request_next_action(endpoint_create);

error:
	return rc;
}

static int endpoint_create_process_request(struct controller_api *endpoint_create,
					 void *args)
{
	int rc = 0;
	struct request *request = NULL;

	request = endpoint_create->request;

	rc = request_validate_headers(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	if (request_content_length(request) == 0) {
		/**
		 * Expecting create request endpoint info.
		 */
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_create_send_response(endpoint_create, NULL);
		goto error;
	}

	/* Set read data call back. */
	request_set_readcb(request, endpoint_create_process_data);

error:
	return rc;
}


static controller_api_action_func default_endpoint_create_actions[] =
{
	endpoint_create_process_request,
	endpoint_create_send_response,
};

static int endpoint_create_init(struct controller *controller,
		   struct request *request,
		   struct controller_api **api)
{
	int rc = 0;
	struct controller_api *endpoint_create = NULL;

	endpoint_create = malloc(sizeof(struct controller_api));
	if (endpoint_create == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	endpoint_create->request = request;
	endpoint_create->controller = controller;

	endpoint_create->name = "CREATE";
	endpoint_create->type = ENDPOINT_CREATE_ID;
	endpoint_create->action_next = 0;
	endpoint_create->action_table = default_endpoint_create_actions;

	endpoint_create->priv = calloc(1, sizeof(struct endpoint_create_api));
	if (endpoint_create->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut parameter value. */
	*api = endpoint_create;
	endpoint_create = NULL;

error:
	if (endpoint_create) {
		if (endpoint_create->priv) {
			free(endpoint_create->priv);
		}

		free(endpoint_create);
	}

	return rc;
}

static void endpoint_create_fini(struct controller_api *endpoint_create)
{
	if (endpoint_create->priv) {
		free(endpoint_create->priv);
	}

	if (endpoint_create) {
		free(endpoint_create);
	}
}

/**
 * ##############################################################
 * #		ENDPOINT DELETE API'S					#
 * ##############################################################
 */
struct endpoint_delete_api_req {
	const char *endpoint_name;
	/* ... */
};

struct endpoint_delete_api_resp {
	int status;
	/* ... */
};

struct endpoint_delete_api {
	struct endpoint_delete_api_req  req;
	struct endpoint_delete_api_resp resp;
};

static int endpoint_delete_send_response(struct controller_api *endpoint_delete,
				       void *args)
{
	int rc = 0;
	int resp_code = 0;
	struct request *request = NULL;

	request = endpoint_delete->request;

	rc = request_get_errcode(request);
	if (rc != 0) {
		resp_code = errno_to_http_code(rc);
	} else {
		resp_code = EVHTP_RES_200;
	}

	request_send_response(request, resp_code);
	return rc;
}

static int endpoint_delete_process_request(struct controller_api *endpoint_delete,
					   void *args)
{
	int rc = 0;
	str256_t endpoint_name;
	int endpoint_name_len = 0;
	struct request *request = NULL;
	struct endpoint_delete_api *endpoint_delete_api = NULL;

	request = endpoint_delete->request;

	rc = request_validate_headers(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		endpoint_delete_send_response(endpoint_delete, NULL);
		goto error;
	}

	if (request_content_length(request) != 0) {
		/**
		 * ENDPOINT delete request doesn't expect any payload data.
		 */
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_delete_send_response(endpoint_delete, NULL);
		goto error;
	}

	/**
	 * Get the ENDPOINT delete api info.
	 */
	endpoint_delete_api = (struct endpoint_delete_api*)endpoint_delete->priv;
	endpoint_delete_api->req.endpoint_name = request_api_file(request);
	if (endpoint_delete_api->req.endpoint_name == NULL) {
		log_err("No FS name.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_delete_send_response(endpoint_delete, NULL);
		goto error;
	}

	/* Compose eendpoint_endpoint_delete api params. */
	log_debug("Deleting ENDPOINT : %s.", endpoint_delete_api->req.endpoint_name);

	endpoint_name_len = strlen(endpoint_delete_api->req.endpoint_name);
	if (endpoint_name_len > 255) {
		log_err("FS name too long.");
		rc = EINVAL;
		request_set_errcode(request, rc);
		endpoint_delete_send_response(endpoint_delete, NULL);
		goto error;
	}

	str256_from_cstr(endpoint_name,
			 endpoint_delete_api->req.endpoint_name,
			 endpoint_name_len);

	/**
	 * Send endpoint delete request to the backend.
	 */
	rc = cfs_endpoint_delete(&endpoint_name);
	request_set_errcode(request, -rc);

	log_debug("ENDPOINT delete return code: %d.", rc);

	request_next_action(endpoint_delete);

error:
	return rc;
}

static controller_api_action_func default_endpoint_delete_actions[] =
{
	endpoint_delete_process_request,
	endpoint_delete_send_response,
};

static int endpoint_delete_init(struct controller *controller,
		  struct request *request,
		  struct controller_api **api)
{
	int rc = 0;
	struct controller_api *endpoint_delete = NULL;

	endpoint_delete = malloc(sizeof(struct controller_api));
	if (endpoint_delete == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	endpoint_delete->request = request;
	endpoint_delete->controller = controller;

	endpoint_delete->name = "DELETE";
	endpoint_delete->type = ENDPOINT_DELETE_ID;
	endpoint_delete->action_next = 0;
	endpoint_delete->action_table = default_endpoint_delete_actions;

	endpoint_delete->priv = calloc(1, sizeof(struct endpoint_delete_api));
	if (endpoint_delete->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut Parameter value. */
	*api = endpoint_delete;
	endpoint_delete = NULL;

error:
	if (endpoint_delete) {
		if (endpoint_delete->priv) {
			free(endpoint_delete->priv);
		}

		free(endpoint_delete);
	}

	return rc;
}

static void endpoint_delete_fini(struct controller_api *endpoint_delete)
{
	if (endpoint_delete->priv) {
		free(endpoint_delete->priv);
	}

	if (endpoint_delete) {
		free(endpoint_delete);
	}
}

/**
 * ##############################################################
 * #		ENDPOINT CONTROLLER API'S				#
 * ##############################################################
 */
#define ENDPOINT_NAME	"endpoint"
#define ENDPOINT_API_URI	"/endpoint"

static char *default_endpoint_api_list[] =
{
#define XX(uc, lc, _)	#lc,
	ENDPOINT_API_MAP(XX)
#undef XX
};

static struct controller_api_table endpoint_api_table [] =
{
#define XX(uc, lc, method)	{ #lc, #method, ENDPOINT_ ## uc ## _ID },
	ENDPOINT_API_MAP(XX)
#undef XX
};

static int endpoint_api_name_to_id(char *api_name, enum endpoint_api_id *api_id)
{
	int rc = EINVAL;
	int idx = 0;

	for (idx = 0; idx < ENDPOINT_API_COUNT; idx++) {
		if (!strcmp(endpoint_api_table[idx].method, api_name)) {
			*api_id = endpoint_api_table[idx].id;
			rc = 0;
			break;
		}
	}

	return rc;
}

static int endpoint_api_init(char *api_name,
	       struct controller *controller,
	       struct request *request,
	       struct controller_api **api)
{
	int rc = 0;
	enum endpoint_api_id api_id;
	struct controller_api *endpoint_api = NULL;

	rc = endpoint_api_name_to_id(api_name, &api_id);
	if (rc != 0) {
		log_err("Unknown endpoint api : %s.\n", api_name);
		goto error;
	}

	switch(api_id) {
#define XX(uc, lc, _)							      \
	case ENDPOINT_ ## uc ## _ID:					      \
		rc = endpoint_ ## lc ## _init(controller, request, &endpoint_api);\
		break;
		ENDPOINT_API_MAP(XX)
#undef XX
	default:
		log_err("Not supported api : %s", api_name);
	}

	/* Assign the InOut variable api value. */
	*api = endpoint_api;

error:
	return rc;
}

static void endpoint_api_fini(struct controller_api *endpoint_api)
{
	char *api_name = NULL;
	enum endpoint_api_id api_id;

	api_name = endpoint_api->name;
	api_id = endpoint_api->type;

	switch(api_id) {
#define XX(uc, lc, _)							\
	case ENDPOINT_ ## uc ## _ID:					\
		endpoint_ ## lc ## _fini(endpoint_api);			\
		break;
		ENDPOINT_API_MAP(XX)
#undef XX
	default:
		log_err("Not supported api : %s", api_name);
	}
}

static struct controller default_endpoint_controller =
{
	.name	  = ENDPOINT_NAME,
	.type	  = CONTROLLER_ENDPOINT_ID,
	.api_uri  = ENDPOINT_API_URI,
	.api_list = default_endpoint_api_list,
	.api_init = endpoint_api_init,
	.api_fini = endpoint_api_fini,
};

int ctl_endpoint_init(struct server *server, struct controller **controller)
{
	int rc = 0;

	struct controller *endpoint_controller = NULL;

	endpoint_controller = malloc(sizeof(struct controller));
	if (endpoint_controller == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Init endpoint_controller. */
	*endpoint_controller = default_endpoint_controller;
	endpoint_controller->server = server;

	/* Assign the return valure. */
	*controller = endpoint_controller;

error:
	return rc;
}

void ctl_endpoint_fini(struct controller *endpoint_controller)
{
	free(endpoint_controller);
	endpoint_controller = NULL;
}
