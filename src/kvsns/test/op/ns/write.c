/**
 * @file write.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh write [mode] <args>.
 *
 * IO utility for writing on kvsns file.
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

#define WRITE_FLAG O_WRONLY
#define WRITE_MODE 0644

static struct option opts[] = {
	{ .name = "mode",	.has_arg = required_argument,	.val = 'm' },
	{ .name = "inode",	.has_arg = required_argument,	.val = 'i' },
	{ .name = "dir-inode",	.has_arg = required_argument,	.val = 'd' },
	{ .name = "offset",	.has_arg = required_argument,	.val = 'p' },
	{ .name = "size",	.has_arg = required_argument,	.val = 's' },
	{ .name = "input",	.has_arg = required_argument,	.val = 'o' },
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
	printf("Usage: %s <WRITE_MODE> [MODE_OPTIONS]...\n"
		"\tWRITE MODE:\n"
		"\t-m, --mode=[INODE|FILE]\tWrite mode[FILE].\n"
		"\tCommon Options:\n"
		"\t-p, --offset=OFFSET\tStart position.\n"
		"\t-s, --size=SIZE\t\tNumber of byttes to read.\n"
		"\t-o, --input=FILE\tInput file(default: stdin).\n"
		"\tMODE INODE:\n"
		"\t-i, --inode=INO\t\tFile inode.\n"
		"\tMODE FILE:\n"
		"\t-d, --dir-inode=INO\tParent directory inode[2].\n"
		"\t<file>\t\t\tFile to write.\n", prog);

	printf("\n");

	printf("Example: %s --mode=INODE -i 3 --offset=4096 --size=1024.\n", prog); 

	exit(EXIT_FAILURE);
}

int op_write(void *ctx, void *cred, int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t *fs_ctx = (kvsns_fs_ctx_t*)ctx;
	kvsns_cred_t *user_cred = (kvsns_cred_t*)cred;

	kvsns_ino_t write_inode = 0;
	kvsns_ino_t dir_inode = KVSNS_ROOT_INODE;

	off_t offset = 0;
	size_t size = 0;
	int fd = STDIN_FILENO;

	int write_mode = WRITE_MODE_FILE;	
	char *input_file = NULL;
	char *fname = NULL;
	int c;

	// Reinitialize getopt internals.
	optind = 0;

	while ((c = getopt_long(argc, argv, "m:i:d:p:s:o:h", opts, NULL)) != -1) {
		switch (c) {
		case 'm':
			rc = parse_write_mode(optarg, &write_mode);
			RC_JUMP_NOTEQ(rc, STATUS_OK, error);
			break;
		case 'd':
			dir_inode = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			write_inode = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			offset = strtoul(optarg, NULL, 0);
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			input_file = strdup(optarg);
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

	if ((write_mode == WRITE_MODE_FILE) &&
			(optind == argc || write_inode != 0)) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	if (write_mode == WRITE_MODE_INODE &&
		(optind != argc || dir_inode != KVSNS_ROOT_INODE)) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	if (write_mode == WRITE_MODE_FILE) {
		// Get the read inode.
		fname = argv[optind];
		RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_lookup, fs_ctx,
				user_cred, &dir_inode, fname, &write_inode);
	}

	if (write_inode == 0) {
		fprintf(stderr, "Bad parameters.\n");
		print_usage = 1;
		goto error;
	}

	printf("Opening file  = [%lld].\n", write_inode);

	kvsns_file_open_t kfd;
	RC_LOG_NOTEQ(rc, STATUS_OK, error, kvsns2_open,
			fs_ctx, user_cred, &write_inode, WRITE_FLAG, WRITE_MODE, &kfd);

	printf("Opened file  = [%lld]\n", kfd.ino);

	/**
	 * open source fd
	 */
	if (input_file) {
		fd = open(input_file, O_RDONLY, 0644);
		if (fd < 0) {
			fprintf(stderr, "Can't open POSIX fd, errno=%u\n", errno);
			fd = STDOUT_FILENO;
		}
	}

	rc = cp_to_kvsns(fs_ctx, user_cred, &kfd, offset, size, fd);

error:
	if (input_file)
		free(input_file);
	if (print_usage)
		usage(argv[0]);

	return rc;
}
