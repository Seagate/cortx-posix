/**
 * @file unlink.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh unlink [<args>] <filename>.
 *
 * Kvssh unlink/rm utility to delete a file.
 * It accept fs_ctx, user_cred, dir_inode and filename. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <option.h>

static struct option opts[] = {
	{ .name = "dir-inode",	.has_arg = required_argument,	.val = 'i' },
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
	printf("Usage: %s [OPTIONS]... fname\n"
		"\t-i, --dir-inode=INO\tParent directory inode[2].\n", prog);

	printf("\n");

	printf("Example: %s -i 2 <filename>.\n", prog); 
	
	exit(EXIT_FAILURE);
}

int op_unlink(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t dir_inode = KVSNS_ROOT_INODE;
	
	char *fname = NULL;
	int c;
	// Reinitialize getopt internals.
	optind = 0;

	while ((c = getopt_long(argc, argv, "i:h", opts, NULL)) != -1) {
		switch (c) {
		case 'i':
			dir_inode = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			usage(argv[0]);
			break;
		default:
			fprintf(stderr, "Bad parameters.\n");
			print_usage = 1;
			rc = -1;
			goto error;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	fname = argv[optind];

	printf("Removing File  = [%s].\n", fname);

	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_unlink,
			fs_ctx, user_cred, &dir_inode, NULL, fname);

	printf("Removed File  = [%s].\n", fname);

error:
	if (print_usage)
		usage(argv[0]);

	return rc;
}
