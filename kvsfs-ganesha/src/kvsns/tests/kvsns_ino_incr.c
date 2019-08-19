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
	unsigned int i = 0;
	unsigned long long counter = 0LL;
	char k[KLEN];
	char v[VLEN];

	if (argc != 1) {
		fprintf(stderr, "%s: No argument required\n", argv[0]);
		return -EINVAL;
	}

	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(1);
	}

	memset(k, 0, KLEN);
	memset(v, 0, VLEN);
	snprintf(k, KLEN, "ino_counter");
	snprintf(v, VLEN, "3");

	/* KVS FUNCTIONS */
	rc = kvsal_set_char(k, v);
	if (rc != 0) {
		fprintf(stderr, "kvsns_set_char: err=%d\n", rc);
		exit(1);
	}

	for (i = 0 ; i < 10 ; i++) {
		rc = kvsal2_incr_counter(NULL, k, &counter);
		if (rc != 0) {
			fprintf(stderr, "kvsns_incr_counter failed: rc=%d\n",
				rc);
			exit(1);
		}
		printf("Iter:%d counter=%llu\n", i, counter);
	}

	printf("SUCCESS\n");

	return 0;
}
