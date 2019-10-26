/**
 * @file read.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh read [mode] <args>.
 *
 * IO utility for reading kvsns file.
 * A file can be identifed by it's inode number or
 * directory inode and filename combination. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <option.h>
#include <cp.h>

#define READ_FLAG O_RDONLY
#define READ_MODE 0644

static struct option opts[] = {
	{ .name = "mode",	.has_arg = required_argument,	.val = 'm' },
	{ .name = "inode",	.has_arg = required_argument,	.val = 'i' },
	{ .name = "dir-inode",	.has_arg = required_argument,	.val = 'd' },
	{ .name = "offset",	.has_arg = required_argument,	.val = 'p' },
	{ .name = "size",	.has_arg = required_argument,	.val = 's' },
	{ .name = "output",	.has_arg = required_argument,	.val = 'o' },
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
	printf("Usage: %s <READ_MODE> [MODE_OPTIONS]...\n"
		"\tREAD MODE:\n"
		"\t-m, --mode=[INODE|FILE]\tRead mode[FILE].\n"
		"\tCommon Options:\n"
		"\t-p, --offset=OFFSET\tStart position.\n"
		"\t-s, --size=SIZE\t\tNumber of byttes to read.\n"
		"\t-o, --output=FILE\tOutput file(default: stdout).\n"
		"\tMODE INODE:\n"
		"\t-i, --inode=INO\t\tFile inode.\n"
		"\tMODE FILE:\n"
		"\t-d, --dir-inode=INO\tParent directory inode[2].\n"
		"\t<file>\t\t\tFile to read.\n", prog);

	printf("\n");

	printf("Example: %s --mode=INODE -i 3 --offset=4096 --size=1024.\n", prog); 

	exit(EXIT_FAILURE);
}

int op_read(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t read_inode = 0;
	kvsns_ino_t dir_inode = KVSNS_ROOT_INODE;

	off_t offset = 0;
	size_t size = 0;
	int fd = STDOUT_FILENO;

	int read_mode = READ_MODE_FILE;	
	char *output_file = NULL;
	char *fname = NULL;
	int c;

	// Reinitialize getopt internals.
	optind = 0;

	while ((c = getopt_long(argc, argv, "m:i:d:p:s:o:h", opts, NULL)) != -1) {
		switch (c) {
		case 'm':
			rc = parse_read_mode(optarg, &read_mode);
			RC_JUMP_NOTEQ(rc, STATUS_OK, error);
			break;
		case 'd':
			dir_inode = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			read_inode = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			offset = strtoul(optarg, NULL, 0);
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			output_file = strdup(optarg);
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

	if ((read_mode == READ_MODE_FILE) &&
			(optind == argc || read_inode != 0)) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	if (read_mode == READ_MODE_INODE &&
		(optind != argc || dir_inode != KVSNS_ROOT_INODE)) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	if (read_mode == READ_MODE_FILE) {
		// Get the read inode.
		fname = argv[optind];
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_lookup, fs_ctx,
				user_cred, &dir_inode, fname, &read_inode);
	}

	if (read_inode == 0) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	printf("Opening file  = [%lld].\n", read_inode);

	kvsns_file_open_t kfd;
	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_open,
			fs_ctx, user_cred, &read_inode, READ_FLAG, READ_MODE, &kfd);

	printf("Opened file  = [%lld]\n", kfd.ino);

	/**
	 * open output_file fd
	 */
	if (output_file) {
		fd = open(output_file, O_WRONLY|O_CREAT, 0644);
		if (fd < 0) {
			fprintf(stderr, "Can't open POSIX fd, errno=%u\n", errno);
			fd = STDOUT_FILENO;
		}
	}

	rc = cp_from_kvsns(fs_ctx, user_cred, &kfd, offset, size, fd);

error:
	if (output_file)
		free(output_file);
	if (print_usage)
		usage(argv[0]);

	return rc;
}
