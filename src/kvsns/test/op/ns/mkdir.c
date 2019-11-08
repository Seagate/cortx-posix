/**
 * @file mkdir.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh mkdir [<args>] dirname.
 *
 * Directories can be directly created using mkdir utility.
 * It accept fs_ctx, user_cred, dir_inode and mode. 
 */

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
	printf("Usage: %s [OPTIONS]... dirname\n"
		"\t-i, --dir-inode=INO\tParent directory inode[2].\n"
		"\t-m, --mode=MODE\t\tDirectory mode bits[0644].\n", prog);

	printf("\n");

	printf("Example: %s -i 2 --mode=S_IRUSR:S_IWUSR <dirname>.\n", prog); 
	
	exit(EXIT_FAILURE);
}

int op_mkdir(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t curr_inode = 0LL;
	kvsns_ino_t dir_inode = KVSNS_ROOT_INODE;
	
	mode_t mode = 0644;
        
	char *cloned_mode = NULL;
	char *dirname = NULL;
	int c;

	// Reinitialize getopt internals.
	optind = 0;

	while ((c = getopt_long(argc, argv, "i:m:h", opts, NULL)) != -1) {
		switch (c) {
		case 'i':
			dir_inode = strtoul(optarg, NULL, 0);
			printf("dir inode : %lld\n", dir_inode);
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

	dirname = argv[optind];

#ifdef DEBUG
	printf("Directory  = %s\n", dirname);
#endif


	printf("Creating directory  = [%s].\n", dirname);

	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns_mkdir,
			fs_ctx, user_cred, &dir_inode, dirname, mode, &curr_inode);

	printf("Created directory  = [%s], inode : %lld.\n", dirname, curr_inode);


error:
	if (cloned_mode)
		free(cloned_mode);
	if (print_usage)
		usage(argv[0]);

	return rc;
}
