/**
 * Filename: auth_setup.c
 * Description: Authentication Setup controller.
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <evhtp.h>
#include <json/json.h>
#include <management.h>
#include <common/log.h>
#include <str.h>
#include <debug.h>
#include "internal/controller.h"

/**
 * ##############################################################
 * #		AUTH_SETUP CREATE API'S				#
 * ##############################################################
 */

struct auth_setup_api_req {
	const char *auth_type;
	const char *ldap_server;
	const char *ldap_baseDN;
	const char *ldap_admin_ac;
	const char *ldap_admin_pw;
	/* ... */
};

struct auth_setup_api_resp {
	/* ... */
};

struct auth_setup_api {
	struct auth_setup_api_req  req;
	struct auth_setup_api_resp resp;
};

static int auth_setup_send_response(struct controller_api *auth_setup,
				       void *args)
{
	int rc = 0;
	int resp_code = 0;
	struct request *request = NULL;

	request = auth_setup->request;

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

static int ldap_setup(struct auth_setup_api *auth_setup_api,
		      struct json_object *json_obj)
{
	int rc = 0;
	struct json_object *jobj_server = NULL;
	struct json_object *jobj_baseDN = NULL;
	struct json_object *jobj_admin_ac = NULL;
	struct json_object *jobj_admin_pw = NULL;
	char cmd[1024];
	bool info_stored = false, auth_set = false, no_backup = false;
	/* various commands */
	const char *auth_cmd = "/sbin/authconfig";
	const char *ldap_opt = "--enableldap --enableldapauth --ldapserver";
	const char *add_binddn = "/usr/bin/sed -i.bak \'4s/^/binddn";
	const char *add_bindpw = "/usr/bin/sed -i.bak \'5s/^/bindpw";
	const char *chng_binddn = "/usr/bin/sed -i.bak \'s/^binddn.*$/binddn";
	const char *chng_bindpw = "/usr/bin/sed -i.bak \'s/^bindpw.*$/bindpw";

	/* Get LDAP auth setup related arguments */
	json_object_object_get_ex(json_obj, "server", &jobj_server);
	json_object_object_get_ex(json_obj, "baseDN", &jobj_baseDN);
	json_object_object_get_ex(json_obj, "admin_account", &jobj_admin_ac);
	json_object_object_get_ex(json_obj, "admin_account_pw", &jobj_admin_pw);

	/* Check for NULL objects */
	if ((jobj_server == NULL) || (jobj_baseDN == NULL) || 
	    (jobj_admin_ac == NULL) || (jobj_admin_pw == NULL)) {
		log_err("Required parameters not provided");
		rc = EINVAL;
		goto err_exit;
	}

	auth_setup_api->req.ldap_server = json_object_get_string(jobj_server);
	auth_setup_api->req.ldap_baseDN = json_object_get_string(jobj_baseDN);
	auth_setup_api->req.ldap_admin_ac =
			json_object_get_string(jobj_admin_ac);
	auth_setup_api->req.ldap_admin_pw =
			json_object_get_string(jobj_admin_pw);

	/* Check for NULL strings */
	if ((auth_setup_api->req.ldap_server == NULL) ||
	    (auth_setup_api->req.ldap_baseDN == NULL) || 
	    (auth_setup_api->req.ldap_admin_ac == NULL) ||
	    (auth_setup_api->req.ldap_admin_pw == NULL)) {
		log_err("Required parameters not provided");
		rc = EINVAL;
		goto err_exit;
	}

	log_debug("LDAP authentication setup with parameters : \n \
		   Server : %s \n baseDN : %s \n admin_account : %s",
		   auth_setup_api->req.ldap_server,
		   auth_setup_api->req.ldap_baseDN,
		   auth_setup_api->req.ldap_admin_ac);

	/* TODO - First check if auth info exists in KVstore */

	/* TODO - Store the auth info in KVstore */
	info_stored = true;

	/* Save existing auth config on node */
	rc = system("/sbin/authconfig --savebackup=cortx_backup");
	if (rc) {
		/* Just log the error. This failure is not that critical */
		log_err("authconfig backup failed with rc=%d", rc);
		no_backup = true;
		rc = 0;
	}
	/* Prepare auth command for the setup task */
	snprintf(cmd, sizeof(cmd) , "%s %s %s --ldapbasedn=\"%s\" --update",
		 auth_cmd, ldap_opt, auth_setup_api->req.ldap_server,
		 auth_setup_api->req.ldap_baseDN );
	log_debug("Command to be executed : %s", cmd);
	/* Carry out auth setup */
	rc = system(cmd);
	if (rc) {
		log_err("authconfig command failed with rc=%d", rc);
		goto err_exit;
	}
	auth_set = true;

	/* Update nslcd.conf file */

	/* check if binddn entry exists */
	rc = system("/usr/bin/grep ^binddn /etc/nslcd.conf > /dev/null");
	if (rc) {
		/* binddn not present add the entry */
		snprintf(cmd, sizeof(cmd) , "%s %s\\n/\' /etc/nslcd.conf",
			 add_binddn, auth_setup_api->req.ldap_admin_ac);
		log_debug("Command to be executed : %s", cmd);
		rc = system(cmd);
		if (rc) {
			log_err("adding binddn entry failed with rc=%d", rc);
			goto err_exit;
		}
	} else {
		/* replace existing binddn */
		snprintf(cmd, sizeof(cmd) , "%s %s /\' /etc/nslcd.conf",
			 chng_binddn, auth_setup_api->req.ldap_admin_ac);
		log_debug("Command to be executed : %s", cmd);
		rc = system(cmd);
		if (rc) {
			log_err("replacing binddn entry failed with rc=%d", rc);
			goto err_exit;
		}
	}
	/* check if bindpw entry exists */
	rc = system("/usr/bin/grep ^bindpw /etc/nslcd.conf > /dev/null");
	if (rc) {
		/* bindpw not present add the entry */
		snprintf(cmd, sizeof(cmd) , "%s %s\\n/\' /etc/nslcd.conf",
			 add_bindpw, auth_setup_api->req.ldap_admin_pw);
		log_debug("Command to be executed : %s", cmd);
		rc = system(cmd);
		if (rc) {
			log_err("adding bindpw entry failed with rc=%d", rc);
			goto err_exit;
		}
	} else {
		/* replace existing bindpw */
		snprintf(cmd, sizeof(cmd) , "%s %s /\' /etc/nslcd.conf",
			 chng_bindpw, auth_setup_api->req.ldap_admin_pw);
		log_debug("Command to be executed : %s", cmd);
		rc = system(cmd);
		if (rc) {
			log_err("replacing bindpw entry failed with rc=%d", rc);
			goto err_exit;
		}
	}

err_exit:
	if (rc) {
		if (info_stored && auth_set) {
			/* Failure in editing nslcd.conf file */
			/* Inform user for taking corrective action mnually */
			log_err("/etc/nslcd.conf file updation failed for \
			     binddn and/or bindpw. Please add those manually");
			rc = 0;
		} else if (info_stored) {
			/* TODO - remove auth info from KVstore */
		} else {
			/* restore back auth info */
			if (no_backup) {
				rc = system("/usr/sbin/authconfig \
				     --restorelastbackup");
			} else {
				rc = system("/usr/sbin/authconfig \
				     --restorebackup=cortx_backup");
			}
		}
	}
	return rc;
}

static int auth_setup_process_data(struct controller_api *auth_setup)
{
	int rc = 0;
	struct request *request = NULL;
	struct auth_setup_api *auth_setup_api = NULL;
	struct json_object *json_obj = NULL;
	struct json_object *jobj_authtype = NULL;

	request = auth_setup->request;

	/**
	 * Process the auth_setup data.
	 * 1. Parse JSON request data.
	 * 2. Compose auth setup api params.
	 * 3. Execute the request
	 * 4. Get response.
	 */
	rc = request_accept_data(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		auth_setup_send_response(auth_setup, NULL);
		goto error;
	}

	/* 1. Parse JSON requst data. */
	auth_setup_api = (struct auth_setup_api*)auth_setup->priv;

	json_obj = request_get_data(request);
	json_object_object_get_ex(json_obj, "type", &jobj_authtype);

	if (jobj_authtype == NULL) {
		log_err("No Auth type provided");
		rc = EINVAL;
		request_set_errcode(request, rc);
		auth_setup_send_response(auth_setup, NULL);
		goto error;
	}
	auth_setup_api->req.auth_type = json_object_get_string(jobj_authtype);

	if (strcmp(auth_setup_api->req.auth_type, "ldap") == 0) {
		rc = ldap_setup(auth_setup_api, json_obj);
		if (rc) {
			request_set_errcode(request, rc);
			auth_setup_send_response(auth_setup, NULL);
			goto error;
		}
	} else {
		/* Unknown setup type */
		log_err("Unknown  Auth type provided");
		rc = EINVAL;
		request_set_errcode(request, rc);
		auth_setup_send_response(auth_setup, NULL);
		goto error;
	}

	log_debug("Auth Setup Completed");

	request_next_action(auth_setup);

error:
	return rc;
}

static int auth_setup_process_request(struct controller_api *auth_setup,
					 void *args)
{
	int rc = 0;
	struct request *request = NULL;

	request = auth_setup->request;

	rc = request_validate_headers(request);
	if (rc != 0) {
		/**
		 * Internal error.
		 */
		request_set_errcode(request, rc);
		auth_setup_send_response(auth_setup, NULL);
		goto error;
	}

	if (request_content_length(request) == 0) {
		/**
		 * Expecting auth setup request info.
		 */
		rc = EINVAL;
		request_set_errcode(request, rc);
		auth_setup_send_response(auth_setup, NULL);
		goto error;
	}

	/* Set read data call back. */
	request_set_readcb(request, auth_setup_process_data);

error:
	return rc;
}


static controller_api_action_func default_auth_setup_actions[] =
{
	auth_setup_process_request,
	auth_setup_send_response,
};

static int auth_setup_init(struct controller *controller,
		   struct request *request,
		   struct controller_api **api)
{
	int rc = 0;
	struct controller_api *auth_setup = NULL;

	auth_setup = malloc(sizeof(struct controller_api));
	if (auth_setup == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	auth_setup->request = request;
	auth_setup->controller = controller;

	auth_setup->name = "SETUP";
	auth_setup->type = AUTH_SETUP_ID;
	auth_setup->action_next = 0;
	auth_setup->action_table = default_auth_setup_actions;

	auth_setup->priv = calloc(1, sizeof(struct auth_setup_api));
	if (auth_setup->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut parameter value. */
	*api = auth_setup;
	auth_setup = NULL;

error:
	if (auth_setup) {
		if (auth_setup->priv) {
			free(auth_setup->priv);
		}

		free(auth_setup);
	}

	return rc;
}

static void auth_setup_fini(struct controller_api *auth_setup)
{
	if (auth_setup->priv) {
		free(auth_setup->priv);
	}

	if (auth_setup) {
		free(auth_setup);
	}
}

/**
 * ##############################################################
 * #		AUTH REMOVE API'S				#
 * ##############################################################
 */
struct auth_remove_api_req {
	/* ... */
};

struct auth_remove_api_resp {
	int status;
	/* ... */
};

struct auth_remove_api {
	struct auth_remove_api_req  req;
	struct auth_remove_api_resp resp;
};

static int auth_remove_send_response(struct controller_api *auth_remove,
				       void *args)
{
	/* placeholder */
	return 0;
}

static int auth_remove_process_request(struct controller_api *auth_remove,
					   void *args)
{
	/* placeholder */
	return 0;
}

static controller_api_action_func default_auth_remove_actions[] =
{
	auth_remove_process_request,
	auth_remove_send_response,
};

static int auth_remove_init(struct controller *controller,
		  struct request *request,
		  struct controller_api **api)
{
	/* placeholder code */
	int rc = 0;
	struct controller_api *auth_remove = NULL;

	auth_remove = malloc(sizeof(struct controller_api));
	if (auth_remove == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Init. */
	auth_remove->request = request;
	auth_remove->controller = controller;

	auth_remove->name = "REMOVE";
	auth_remove->type = AUTH_REMOVE_ID;
	auth_remove->action_next = 0;
	auth_remove->action_table = default_auth_remove_actions;

	auth_remove->priv = calloc(1, sizeof(struct auth_remove_api));
	if (auth_remove->priv == NULL) {
		rc = ENOMEM;
		log_err("Internal error: No memmory.\n");
		goto error;
	}

	/* Assign InOut Parameter value. */
	*api = auth_remove;
	auth_remove = NULL;

error:
	if (auth_remove) {
		if (auth_remove->priv) {
			free(auth_remove->priv);
		}

		free(auth_remove);
	}

	return rc;
}

static void auth_remove_fini(struct controller_api *auth_remove)
{
	/* placeholder code */
	if (auth_remove->priv) {
		free(auth_remove->priv);
	}

	if (auth_remove) {
		free(auth_remove);
	}
}

/**
 * ##############################################################
 * #		AUTH CONTROLLER API'S				#
 * ##############################################################
 */
#define AUTH_NAME	"auth"
#define AUTH_API_URI	"/auth"

static char *default_auth_api_list[] =
{
#define XX(uc, lc, _)	#lc,
	AUTH_API_MAP(XX)
#undef XX
};

static struct controller_api_table auth_api_table [] =
{
#define XX(uc, lc, method)	{ #lc, #method, AUTH_ ## uc ## _ID },
	AUTH_API_MAP(XX)
#undef XX
};

static int auth_api_name_to_id(char *api_name, enum auth_api_id *api_id)
{
	int rc = EINVAL;
	int idx = 0;

	for (idx = 0; idx < AUTH_API_COUNT; idx++) {
		if (!strcmp(auth_api_table[idx].method, api_name)) {
			*api_id = auth_api_table[idx].id;
			rc = 0;
			break;
		}
	}

	return rc;
}

static int auth_api_init(char *api_name,
	       struct controller *controller,
	       struct request *request,
	       struct controller_api **api)
{
	int rc = 0;
	enum auth_api_id api_id;
	struct controller_api *auth_api = NULL;

	rc = auth_api_name_to_id(api_name, &api_id);
	if (rc != 0) {
		log_err("Unknown auth_setup api : %s.\n", api_name);
		goto error;
	}

	switch(api_id) {
#define XX(uc, lc, _)							      \
	case AUTH_ ## uc ## _ID:					      \
		rc = auth_ ## lc ## _init(controller, request, &auth_api);    \
		break;
		AUTH_API_MAP(XX)
#undef XX
	default:
		log_err("Not supported api : %s", api_name);
	}

	/* Assign the InOut variable api value. */
	*api = auth_api;

error:
	return rc;
}

static void auth_api_fini(struct controller_api *auth_api)
{
	char *api_name = NULL;
	enum auth_api_id api_id;

	api_name = auth_api->name;
	api_id = auth_api->type;

	switch(api_id) {
#define XX(uc, lc, _)					\
	case AUTH_ ## uc ## _ID:			\
		auth_ ## lc ## _fini(auth_api);		\
		break;
		AUTH_API_MAP(XX)
#undef XX
	default:
		log_err("Not supported api : %s", api_name);
	}
}

static struct controller default_auth_controller =
{
	.name	  = AUTH_NAME,
	.type	  = CONTROLLER_AUTH_ID,
	.api_uri  = AUTH_API_URI,
	.api_list = default_auth_api_list,
	.api_init = auth_api_init,
	.api_fini = auth_api_fini,
};

int ctl_auth_init(struct server *server, struct controller **controller)
{
	int rc = 0;

	struct controller *auth_controller = NULL;

	auth_controller = malloc(sizeof(struct controller));
	if (auth_controller == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Init auth_controller. */
	*auth_controller = default_auth_controller;
	auth_controller->server = server;

	/* Assign the return valure. */
	*controller = auth_controller;

error:
	return rc;
}

void ctl_auth_fini(struct controller *auth_controller)
{
	free(auth_controller);
	auth_controller = NULL;
}
