/**
 * Filename: test-control.c
 * Description: Control Tester.
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

#define LOG_FILE	"/var/log/eos/control.log"
#define	LOG_LEVEL	LOG_DEBUG

int main(int argc, char *argv[])
{
	int rc = 0;

	 rc = log_init(LOG_FILE, LEVEL_DEBUG);
	 if (rc != 0) {
	 	fprintf(stderr, "Logger init failed, errno : %d.\n", rc);
	 	goto error;
	 }

	rc = server_main(argc, argv);

error:
	return rc;
}

