/**
 * Filename: main.c
 * Description: Main managementi:
 * 		Registers the controllers to the management framework.
 * 		Wait's for the incoming requests and on the new http
 * 		Request invokes the corresponding controller api hadler.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 * 
 * Author: Yogesh Lahane <yogesh.lahane@seagate.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h> /*struct option is defined here */

/* Utils headers. */
#include <management.h>
#include <common/log.h>
#include <debug.h>

/* Local headers. */
#include <controller.h>

pthread_t g_server_thread_id = 0;

/**
 * @brief Print's usage.
 *
 * @param[in] prog 	program name
 */
static void usage(const char *prog)
{
	printf("Usage: %s [OPTIONS]...\n"
		"OPTIONS:\n"
		"\t-p, --port\tControl server port number.\n"
		"\t-r, --reuse-port\tReuse port number for ipv6.\n"
		"\t-b, --bind-ipv6\tBind to ipv6 addr.\n"
		"\t-h, --help\t\tUser help.\n", prog);
}

int server_main(int argc, char *argv[])
{
	int rc = 0;

	struct server *server = NULL;
	struct params *params = NULL;
	struct controller *controller = NULL;

	/* Get params. */
	params = params_parse(argc, argv);
	if (params->print_usage) {
		rc = 1;
		usage(argv[0]);
		goto free_params;
	}

	/**
	 * @TODO: Enable logger when control server is run in process mode.
	 * rc = log_init(params->log_file, params->log_level);
	 * if (rc != 0) {
	 * 	fprintf(stderr, "Logger init failed, errno : %d.\n", rc);
	 * 	goto free_params;
	 * }
	 */

	/* Get control sever instance. */
	server = malloc(sizeof(struct server));
	if (server == NULL) {
		rc = 1;
		log_err("Server instance malloc failed. Exiting..\n");
		goto free_params;
	}

	/* Init Control Server. */
	rc = server_init(server, params);
	if (rc != 0) {
		log_err("Server init failed. Exiting..\n");
		goto free_server;
	}

	/**
	 * Register controllers:
	 * 1. Get the controller instance.
	 * 2. Register it.
	 */
#define XX(uc, lc)					\
	rc = ctl_ ## lc ## _init(server, &controller);	\
	dassert(rc == 0);				\
	dassert(controller != NULL);			\
	controller_register(server, controller);
	CONTROLLER_MAP(XX)
#undef XX

	/* Start Control Server. */
	rc = server_start(server);
	if (rc != 0) {
		log_err("Server start failed. Exiting..\n");
		rc = server_cleanup(server);
		goto exit;
	}

free_server:
	free(server);
free_params:
	free(params);
exit:
	return rc;
}

void* server_thread_start(void *args)
{
	static int rc = 0;

	static int argc = 1;
	static char *argv[] =
	{
		[0] = "control_server",
	};

	rc = server_main(argc, argv);

	return &rc;
}
 
int management_init()
{
	int rc = 0;

	pthread_attr_t attr;
	pthread_t thread_id;

	pthread_attr_init(&attr);

	rc = pthread_create(&thread_id, &attr, &server_thread_start, NULL);
	
	/* Set global thread id. */
	g_server_thread_id = thread_id;

	return rc;
}

int management_cleanup()
{
	int rc = 0;

	void *res;

	/**TODO:
	 * Stop the management server.
	 * Do cleanup it.
	 */

	/* Send thread cancel signal. */
	rc = pthread_cancel(g_server_thread_id);
	if (rc != 0) {
		goto error;
	}

	/* Wait for thread to terminate. */
	rc = pthread_join(g_server_thread_id, &res);

error:
	return rc;
}
