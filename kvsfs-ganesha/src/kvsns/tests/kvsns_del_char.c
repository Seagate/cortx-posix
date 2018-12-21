/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_test.c
 * KVSNS: simple test
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

/* Required Protoype */

int main(int argc, char *argv[])
{
	int rc;
	char *k;

	if (argc != 2) {
		fprintf(stderr, "%s <key>\n", argv[0]);
		return -EINVAL;
	}

	k = malloc(KLEN);
	if (k == NULL)
		return -ENOMEM;

	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(1);
	}

	/* KVS FUNCTIONS */
	strncpy(k, argv[1], KLEN);
	rc = kvsal_del(k);
	if (rc != 0) {
		fprintf(stderr, "kvsns_del: err=%d\n", rc);
		exit(1);
	}
	printf("SUCCESS\n");

	return 0;
}
