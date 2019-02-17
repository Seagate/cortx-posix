/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <libgen.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

int main(int argc, char *argv[])
{
	int rc;
	kvsns_cred_t cred;

	char current_path[MAXPATHLEN];
	char prev_path[MAXPATHLEN];
	char exec_name[MAXPATHLEN];
	char k[KLEN];
	char v[VLEN];

	kvsns_ino_t current_inode = 0LL;
	kvsns_ino_t parent_inode = 0LL;
	kvsns_ino_t ino = 0LL;
	uint64_t fs_id = 1;

	cred.uid = getuid();
	cred.gid = getgid();

	strncpy(exec_name, basename(argv[0]), MAXPATHLEN);

	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_start: err=%d\n", rc);
		exit(1);
	}

	strncpy(k, "KVSNS_INODE", KLEN);
	if (kvsal_get_char(k, v) == 0)
		sscanf(v, "%llu", &current_inode);

	strncpy(k, "KVSNS_PARENT_INODE", KLEN);
	if (kvsal_get_char(k, v) == 0)
		sscanf(v, "%llu", &parent_inode);

	strncpy(k, "KVSNS_PATH", KLEN);
	if (kvsal_get_char(k, v) == 0)
		strncpy(current_path, v, MAXPATHLEN);

	strncpy(k, "KVSNS_PREV_PATH", KLEN);
	if (kvsal_get_char(k, v) == 0)
		strncpy(prev_path, v, MAXPATHLEN);

	printf("exec=%s -- ino=%llu, parent=%llu, path=%s prev=%s\n",
		exec_name, current_inode, parent_inode,
		current_path, prev_path);

	/* Look at exec name and see what is to be done */
	if (!strcmp(exec_name, "kvsns_init")) {
		rc = kvsns_init_root(1); /* Open Bar */
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strncpy(k, "KVSNS_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strncpy(k, "KVSNS_PARENT_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strncpy(k, "KVSNS_PATH", KLEN);
		strncpy(v, "/", VLEN);
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}

		strncpy(k, "KVSNS_PREV_PATH", KLEN);
		strncpy(v, "/", VLEN);
		rc = kvsal_set_char(k, v);
		if (rc != 0) {
			fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
			exit(1);
		}
	} else if (!strcmp(exec_name, "kvsns_create")) {
		if (argc != 2) {
			fprintf(stderr, "creat <newfile>\n");
			exit(1);
		}
		rc = kvsns_create(&fs_id, &cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else
		fprintf(stderr, "%s does not exists\n", exec_name);
	printf("######## OK ########\n");
	return 0;
}
