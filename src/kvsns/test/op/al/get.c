/**
 * @file get.c
 * @author Jatinder Kumar <jatinder.kumar@seagate.com>
 * @brief kvssh get key.
 *
 * Vlaue for given can be directly obtained using get utility.
 * It accept fs_ctx, user_cred.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <option.h>

static struct option opts[] = {
	{ .name = "key", 	.has_arg = required_argument,	.val = 'k' },
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
		"\t-k, --key=KEY \n", prog);

	printf("\n");

	exit(EXIT_FAILURE);
}

int op_get(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;

	char key[KLEN];
	char value[VLEN];
	int c;

	memset(key, 0, KLEN);
	memset(value, 0, VLEN);

	// Reinitialize getopt internals.
	optind = 0;
	while ((c = getopt_long(argc, argv, "k:h", opts, NULL)) != -1) {
		switch (c) {
			case 'k':
				strcpy(key, optarg);
				break;
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

	if (optind != argc || key ==  NULL) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

#ifdef DEBUG
	printf("key  = %s\n", key);
#endif
	int klen = strlen(key);

	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsal2_get_bin,
			fs_ctx, key, klen, value, VLEN);

	printf("key:value  = [%s:%s].\n", key, value);
error:
	if (print_usage)
		usage(argv[0]);

	return rc;
}
