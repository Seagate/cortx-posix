/**
 * @file create.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh create [<args>] filename.
 *
 * Files can be directly created using kvssh <create> utility.
 * It accept fs_ctx, user_cred, dir_inode and mode. 
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for O_DIRECTORY and O_DIRECT */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <option.h>

static struct option opts[] = {
	{ .name = "dir-inode",	.has_arg = required_argument,	.val = 'i' },
	{ .name = "mode",	.has_arg = required_argument,	.val = 'm' },
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
	printf("Usage: %s [OPTIONS]... filename\n"
		"\t-i, --dir-inode=INO\tParent directory inode[2].\n"
		"\t-m, --mode=MODE\t\tFile mode bits[0644].\n", prog);

	printf("\n");
	
	printf("Example: %s -i 2 --mode=S_IRUSR:S_IWUSR <filename>.\n", prog); 
	
	exit(EXIT_FAILURE);
}

int op_create(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t curr_inode = 0LL;
	kvsns_ino_t dir_inode = KVSNS_ROOT_INODE;

	mode_t mode = 0644;

	char *cloned_mode = NULL;
	char *fname;
	int c;

	// Reinitialize getopt internals.
	optind = 0;

	while ((c = getopt_long(argc, argv, "i:m:h", opts, NULL)) != -1) {
		switch (c) {
		case 'i':
			dir_inode = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			cloned_mode = strdup(optarg);
			rc = parse_mode(cloned_mode, &mode);
			RC_JUMP_NOTEQ(rc, STATUS_OK, error);
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

#ifdef DEBUG
	printf("file  = %s\n", fname);
#endif
	printf("Creating file  = [%s].\n", fname);

	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns_creat,
			fs_ctx, user_cred, &dir_inode, fname, mode, &curr_inode);

	printf("Created file  = [%s], inode : %lld.\n", fname, curr_inode);

error:
	if (cloned_mode)
		free(cloned_mode);
	if (print_usage)
		usage(argv[0]);

	return rc;
}
