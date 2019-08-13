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
	if (!strcmp(exec_name, "ns_reset")) {
		strncpy(k, "KVSNS_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		current_inode = KVSNS_ROOT_INODE;
		kvsal_set_char(k, v);

		strncpy(k, "KVSNS_PARENT_INODE", KLEN);
		snprintf(v, VLEN, "%llu", KVSNS_ROOT_INODE);
		parent_inode = KVSNS_ROOT_INODE;
		kvsal_set_char(k, v);

		strncpy(k, "KVSNS_PATH", KLEN);
		strncpy(v, "/", VLEN);
		kvsal_set_char(k, v);

		strncpy(k, "KVSNS_PREV_PATH", KLEN);
		strncpy(v, "/", VLEN);
		kvsal_set_char(k, v);
	} else if (!strcmp(exec_name, "ns_init")) {
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
	} else if (!strcmp(exec_name, "ns_truncate")) {
		kvsns_ino_t fino = 0LL;
		struct stat stat;
		memset(&stat, 0, sizeof(stat));

		if (argc != 3) {
			fprintf(stderr, "truncate <file> <offset>\n");
			exit(1);
		}

		assert(0);
		rc = kvsns2_lookup(NULL, &cred, &current_inode, argv[1], &fino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			exit(1);
		}

		stat.st_size = atoi(argv[2]);

		assert(0);
		rc = kvsns2_setattr(NULL, &cred, &fino, &stat, STAT_SIZE_SET);
		if (rc == 0)
			printf("Truncate %llu/%s %d OK\n", current_inode,
			       argv[1], atoi(argv[2]));
		else
			fprintf(stderr, "Can't unlink %llu/%s rc=%d\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "ns_setxattr")) {
		if (argc != 4) {
			printf("ns_sexattr name xattr_name xattr_val\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			assert(0);
			rc = kvsns2_lookup(NULL, &cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_setxattr(&cred, &ino, argv[2],
				    argv[3], strnlen(argv[3], KLEN)+1, 0);
		if (rc == 0)
			printf("ns_setxattr: %llu --> %s = %s CREATED\n",
				ino, argv[2], argv[3]);
		else
			fprintf(stderr, "Failed : %d\n", rc);
	 } else if (!strcmp(exec_name, "ns_getxattr")) {
		char value[VLEN];
		size_t xattr_size = KLEN;

		if (argc != 3) {
			printf("ns_gexattr name xattr_name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			assert(0);
			rc = kvsns2_lookup(NULL, &cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_getxattr(&cred, &ino, argv[2], value, &xattr_size);
		if (rc == 0)
			printf("ns_getxattr: %llu --> %s = %s\n",
				ino, argv[2], value);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "ns_removexattr")) {
		if (argc != 3) {
			printf("ns_removexattr name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			assert(0);
			rc = kvsns2_lookup(NULL, &cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}
		rc = kvsns_removexattr(&cred, &ino, argv[2]);
		if (rc == 0)
			printf("ns_removexattr: %llu --> %s DELETED\n",
				ino, argv[2]);
		else
			fprintf(stderr, "Failed : %d\n", rc);

	} else if (!strcmp(exec_name, "ns_listxattr")) {
		int offset;
		int size;
		int i = 0;
		kvsns_xattr_t dirent[10];

		if (argc != 2) {
			printf("ns_listxattr name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			assert(0);
			rc = kvsns2_lookup(NULL, &cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		offset = 0;
		size = 10;
		do {
			rc = kvsns_listxattr(&cred, &ino, offset,
					     dirent, &size);
			if (rc != 0) {
				printf("==> readdir failed rc=%d\n", rc);
				exit(1);
			}
			printf("===> size = %d\n", size);
			for (i = 0; i < size; i++)
				printf("%d %s -> xattr:%s\n",
					offset+i, current_path, dirent[i].name);

			if (size == 0)
				break;

			offset += size;
			size = 10;
		} while (1);
	} else if (!strcmp(exec_name, "ns_clearxattr")) {
		if (argc != 2) {
			printf("ns_clearxattr name\n");
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			assert(0);
			rc = kvsns2_lookup(NULL, &cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns_remove_all_xattr(&cred, &ino);
		if (rc == 0)
			printf("All xattr deleted for %s = %llu\n",
			       argv[1], ino);
		else
			fprintf(stderr, "Failed !!!\n");
	} else
		fprintf(stderr, "%s does not exists\n", exec_name);
	printf("######## OK ########\n");
	return 0;
}
