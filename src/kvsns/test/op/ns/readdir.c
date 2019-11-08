/**
 * @file readdir.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh readdir [<args>].
 *
 * Kvssh readdir utility to list directory content.
 * It accept fs_ctx, user_cred, parent_dir_inode and mode. 
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

struct readdir_ctx {
	int index;
};

static bool readdir_cb(void *ctx, const char *name, const kvsns_ino_t *ino)
{
	struct readdir_ctx *readdir_ctx = ctx;

	printf("READDIR:\t\"%d\" \"%s\" \"%llu\"\n",
			readdir_ctx->index, name, *ino);

	readdir_ctx->index++;

	return true;
}

/**
 * @brief Print's usage.
 * 
 * @param[in] prog 	program name
 */
static void usage(const char *prog)
{
	printf("Usage: %s [OPTIONS]... dirname\n"
		"\t-i, --dir-inode=INO\tParent directory inode[2].\n", prog);
	
	printf("\n");

	printf("Example: %s --dir-inode=2.\n", prog); 

	exit(EXIT_FAILURE);
}

int op_readdir(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t dir_inode = KVSNS_ROOT_INODE;

	struct readdir_ctx readdir_ctx[1] = {
		{.index = 0,}
	};
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
	
	if (optind != argc) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	printf("READDIR BEGIN: [%llu] INDEX -> NAME -> INODE\n", dir_inode);

	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns_readdir,
			fs_ctx, user_cred, &dir_inode, readdir_cb, readdir_ctx);

	printf("READDIR END.\n");

error:
	if (print_usage)
		usage(argv[0]);

	return rc;
}
