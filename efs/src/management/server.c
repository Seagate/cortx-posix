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
#include <pthread.h>

/* Utils headers. */
#include <management.h>
#include <common/log.h>
#include <debug.h>

/* Local headers. */
#include "internal/controller.h"

struct server	*g_server_ctx = NULL;
pthread_t	 g_server_tid;     /* Thread id. */
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

int management_start(int argc, char *argv[])
{
	int rc = 0;

	struct server *server = NULL;
	struct controller *controller = NULL;

	/**
	 * Get control sever instance and init.
	 */
	rc = server_init(argc, argv, &server);
	if (rc != 0) {
		if (rc == EINVAL) {
			/* Print Usage. */
			usage(argv[0]);
		}

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

	g_server_ctx = server;

	/* Start Control Server. */
	rc = server_start(g_server_ctx);
	if (rc == 0) {
		log_debug("Exiting server normally.");
	} else {
		log_err("Exiting server with error, rc : %d.", rc);
	}

	/**
	 * 1. Unregister controllers.
	 * 2. Delete the controller object.
	 */
#define XX(uc, lc)						\
	controller = controller_find_by_name(g_server_ctx, #lc);\
	dassert(controller != NULL);				\
	controller_unregister(controller);			\
	ctl_ ## lc ## _fini(controller);
	CONTROLLER_MAP(XX)
#undef XX

free_server:
	server_fini(server);
	return rc;
}

int management_stop()
{
	int rc = 0;

	log_info("Shutting down management service...");

	/**
	 * Stop the management server.
	 * Do cleanup it.
	 */
	rc = server_stop(g_server_ctx);
	if (rc != 0) {
		log_err("Failed to stop management server.!");
	}

	return rc;
}

void* management_thread_start(void *args)
{
	static int rc = 0;

	static int argc = 1;
	static char *argv[] =
	{
		[0] = "control_server",
	};

	rc = management_start(argc, argv);

	log_debug("Exiting Management service thread");

	return &rc;
}

int management_thread_stop()
{
	int rc = 0;

	/**
	 * Shutdown it forcefully.
	 * Send thread cancel signal.
	 */
	rc = pthread_cancel(g_server_tid);
	if (rc != 0) {
		log_err("Management force stop failed.");
		goto error;
	}
error:
	return rc;
}

int management_init()
{
	int rc = 0;

	pthread_attr_t attr;
	pthread_t thread_id;

	pthread_attr_init(&attr);

	log_info("Starting Management service...");

	rc = pthread_create(&thread_id, &attr, &management_thread_start, NULL);
	g_server_tid = thread_id;

	return rc;
}

int management_fini()
{
	int rc = 0;
	void *res;

	rc = management_stop();
	if (rc != 0) {
		log_err("Failed to stop management service.");

		/* Shutdown it forcefully. */
		rc = management_thread_stop();
		if (rc != 0) {
			log_err("Failed to force stop management service.");
		}
	}

	log_debug("Waiting management service thread to terminate."),

	/* Wait for thread to terminate. */

	rc = pthread_join(g_server_tid, &res);
	if (rc != 0 ) {
		log_err("Failed to join management thread.");
	}

	log_info("Shutdown status : %d.", rc);
	return rc;
}
