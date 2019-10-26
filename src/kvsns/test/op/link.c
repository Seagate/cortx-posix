/**
 * @file link.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh link [<args>].
 *
 * Kvssh link utility to create symbolic and hard links.
 * It accept fs_ctx, user_cred, tdir_inode, ldir_inode, target and link_name. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <option.h>

static struct option opts[] = {
	{ .name = "tdir-inode",	.has_arg = required_argument,	.val = 't' },
	{ .name = "ldir-inode",	.has_arg = required_argument,	.val = 'l' },
	{ .name = "symbolic",	.has_arg = no_argument,		.val = 's' },
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
		"\t-t, --tdir-inode=INO\tTarget directory inode[2].\n"
		"\t-l, --ldir-inode=INO\tTarget directory inode[2].\n"
		"\t-s, --symbolic\t\tmake symbolic links instead of hard links.\n",
		prog);

	printf("\n");

	printf("Example: %s --tdir-inode=2 <Target> <LinkName>. (1st form)\n",
		prog); 
	printf("Example: %s --tdir-inode=2 --ldir-inode=2 <Target> <LinkName>. (2nd form)\n", prog); 

	exit(EXIT_FAILURE);
}

int op_link(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t tdir_inode = KVSNS_ROOT_INODE;
	kvsns_ino_t ldir_inode = KVSNS_ROOT_INODE;
	kvsns_ino_t curr_inode = 0LL;
	kvsns_ino_t target_inode = 0LL;

	char *target = NULL;
	char *lname = NULL;
	int symbolic_link = 0;
	int c;

	// Reinitialize getopt internals.
	optind = 0;

	while ((c = getopt_long(argc, argv, "i:t:l:sh", opts, NULL)) != -1) {
		switch (c) {
		case 't':
			tdir_inode = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			ldir_inode = strtoul(optarg, NULL, 0);
			break;
		case 's':
			symbolic_link = 1;
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
	
	if ((argc - optind) != 2) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	if (symbolic_link && ldir_inode != KVSNS_ROOT_INODE) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	target = argv[optind];
	optind++;
	lname = argv[optind];

	printf("Creating Link [%s] -> [%s].\n", lname, target);
	
	if (symbolic_link) {
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns_symlink,
			fs_ctx, user_cred, &tdir_inode, lname, target, &curr_inode);
	} else {
		// Creating a hard link.
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_lookup, fs_ctx,
			user_cred, &tdir_inode, target, &target_inode);
		
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns_link, fs_ctx, user_cred,
			&target_inode, &ldir_inode, lname);
		
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_lookup, fs_ctx,
			user_cred, &ldir_inode, lname, &curr_inode);
	}
		
	printf("Created Link : [%s] Inode :[%llu].\n", lname, curr_inode);

error:
	if (print_usage)
		usage(argv[0]);

	return rc;
}
