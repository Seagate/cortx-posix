/**
 * @file inc_counter.c
 * @author Jatinder Kumar <jatinder.kumar@seagate.com>
 * @brief kvssh set key value.
 *
 * kvsns_incr_counter
 * It accept fs_ctx, user_cred.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <option.h>

static struct option opts[] = {
	{ .name = "help",	.has_arg = no_argument,		.val = 'h' },
	{ .name = NULL }
};

/**
 * @brief Print's usage.
 *
 * @param[in] prog 	program name
 */
static void usage(const char *prog)
{
	printf("Usage: %s\n"
		"\n", prog);

	exit(EXIT_FAILURE);
}

int op_inc_count(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;

	char *kvsal_key = NULL;
	char key[KLEN];
	char value[VLEN];
	int klen = 0;
	int vlen = 0;
	int c;
	unsigned long long count = 0LL;
	unsigned long long i;

	memset(key, 0, KLEN);
	memset(value, 0, VLEN);

	snprintf(key, KLEN, "ino_counter");
        snprintf(value, VLEN, "3");

	// Reinitialize getopt internals.
	optind = 0;
	while ((c = getopt_long(argc, argv, ":h", opts, NULL)) != -1) {
		switch (c) {
			case 'h':
				usage(argv[0]);
				break;
			default:
				fprintf(stderr, "defualt Bad parameters.\n");
				print_usage = 1;
				rc = -1;
				goto error;
		}
	}

	if (optind != argc ) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	klen = strlen(key);
	vlen = strlen(value);

	RC_WRAP_LABEL(rc, free_error, kvsal_alloc, (void **)&kvsal_key,
			klen);

	strcpy(kvsal_key, key);

	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsal2_set_bin,
			fs_ctx, kvsal_key, klen, (void *)value, vlen);

	printf("Set key:value  = [%s:%s].\n", key, value);

	for (i = 0 ; i < 10 ; i++) {
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsal2_incr_counter,
				fs_ctx, kvsal_key, &count);

		printf("Iter:%llu counter=%llu\n", i, count);
	}

free_error:
	kvsal_free(kvsal_key);

error:
	if (print_usage)
		usage(argv[0]);

	return rc;
}
