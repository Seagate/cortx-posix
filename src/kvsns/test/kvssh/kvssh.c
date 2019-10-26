/**
 * @file kvssh.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief kvssh: kvssh shell for ns and io operations.
 *
 * kvssh shell is command line utility to do namespace and io operations
 * directly on kvs store..
 * It accept fs-ctx, user-cred, and commands.
 * The most commonly used [ns] commands are:
 * create, open, openat, mkdir, rmdir, readdir...
 * The most commonly used [io] commands are:
 * read, write...;
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h> /*struct option is defined here*/
#include <eosfs.h>
#include <error.h>
#include <command.h>
#include <option.h>

struct option opts[] = {
	{ .name = "fs-id",	.has_arg = required_argument,	.val = 'F' },
	{ .name = "user-cred",	.has_arg = required_argument,	.val = 'U' },
	{ .name = "list",	.has_arg = no_argument,		.val = 'l' },
	{ .name = "help",	.has_arg = no_argument,		.val = 'h' },
	{ .name = NULL }
};

/**
 * @brief Print's usage.
 * 
 * @param[in] prog 	program name
 */
void usage(const char *prog)
{
	printf("Usage: %s [OPTIONS]... <command> [<args>]\n"
		"\nThe basic user options for all kvsns, with defaults in [ ]:\n"
		"\t-F, --fs-id=FSID\tFilesystem context/id? [0]\n"
		"\t-U, --user-cred=UID:GID\tUser id and group id? [0:0]\n"
		"\t-l, --list\t\tKvssh command list.\n", prog);
	
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int rc;
	int print_usage = 0;

	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;
	uint64_t fs_id = KVSNS_FS_ID_DEFAULT;

	kvsns_cred_t user_cred;
	user_cred.uid = 0;
	user_cred.gid = 0;
	
	kvsns_command_t *command = NULL;
	char *cloned_command = NULL;
	char *cloned_user_cred = NULL;
        int c;
        while ((c = getopt_long(argc, argv, "+F:U:lh", opts, NULL)) != -1) {
		switch (c) {
		case 'F':
			fs_id = strtoul(optarg, NULL, 0);
			break;
		case 'U':
			cloned_user_cred = strdup(optarg);
			rc = parse_user_cred(cloned_user_cred, &user_cred);
			RC_JUMP_NOTEQ(rc, STATUS_OK, error);
			break;
		case 'l':
			display_commands();
			goto error;
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
	
	/**
	 * @TODO Add support getting commands from argv[0],
	 * Which are basically executable links to knssh with command name's.
	 * example link - kvssh_mdir -> kvssh.
	 */

	cloned_command = strdup(argv[optind]);
	rc = parse_command(cloned_command, &command);
	RC_JUMP_NOTEQ(rc, STATUS_OK, error);
	
	optind++;

#ifdef DEBUG
	printf("Command  = %s\n", command->name);
#endif
	if (user_cred.uid == 0 && user_cred.gid == 0) {
		user_cred.uid = getuid();
		user_cred.gid = getgid();
	}

	eos_fs_init();
	
	eos_fs_open(fs_id, &fs_ctx);

	if (command->handle) {
		rc = command->handle(fs_ctx, &user_cred, argc-optind+1, argv+optind-1);
		RC_JUMP_NOTEQ(rc, STATUS_OK, eos_fini);
	}

eos_fini:
	eos_fs_fini();
error:	
	if (cloned_user_cred)
		free(cloned_user_cred);
	if (print_usage)
		usage(argv[0]);

	return rc;
}
