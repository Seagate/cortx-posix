#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>
#include <ini_config.h>

#define KVSNS_DEFAULT_CONFIG "/etc/kvsns.d/kvsns.ini"

int main(int argc, char *argv[])
{
	int rc;
	int i;
	int howmany;
	char key[KLEN];
	char val[VLEN];
	struct collection_item *errors = NULL;
	struct collection_item *cfg_items;

	if (argc != 4) {
		fprintf(stderr, "pattern val how_many args\n");
		exit(1);
	}

	howmany = atoi(argv[3]);

	rc = config_from_file("libkvsns", KVSNS_DEFAULT_CONFIG, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		fprintf(stderr, "Can't read config rc=%d\n", rc);
		free_ini_config_errors(errors);
		return -rc;
	}

	rc = kvsal_init(cfg_items);
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}

	rc = kvsal_begin_transaction();
	if (rc != 0) {
		fprintf(stderr, "kvsal_begin_transaction: err=%d\n", rc);
		exit(-rc);
	}

	for (i = 0; i < howmany; i++) {
		snprintf(key, KLEN, "%s%d", argv[1], i);
		strncpy(val, argv[2], VLEN);
		rc = kvsal_set_char(key, val);
		if (rc != 0) {
			fprintf(stderr, "kvsal_set_char: err=%d\n", rc);
			exit(-rc);
		}
	}

	rc = kvsal_end_transaction();
	if (rc != 0) {
		fprintf(stderr, "kvsal_end_transaction: err=%d\n", rc);
		exit(-rc);
	}

	rc = kvsal_fini();
	if (rc != 0) {
		fprintf(stderr, "kvsal_init: err=%d\n", rc);
		exit(-rc);
	}

	printf("+++++++++++++++\n");

	exit(0);
	return 0;
}
