/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_file_test_write.c
 * KVSNS: simple write test
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

#define SIZE 1024

int main(int argc, char *argv[])
{
	int rc;
	kvsns_ino_t ino = 0LL;
	kvsns_ino_t parent = 0LL;
	kvsns_file_open_t fd;
	kvsns_cred_t cred;
	ssize_t written, read;
	char buff[SIZE], read_buff[SIZE];
	size_t count;
	off_t offset;
	uint64_t fs_id = 1;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;

	cred.uid = getuid();
	cred.gid = getgid();

	printf("uid=%u gid=%u, pid=%d\n",
		getuid(), getgid(), getpid());

	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init: err=%d\n", rc);
		exit(1);
	}

	rc = kvsns_init_root(1);
	if (rc != 0) {
		fprintf(stderr, "kvsns_init_root: err=%d\n", rc);
		exit(1);
	}

	rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
	if (rc != 0) {
		printf("Unable to create index for fs_id:%lu,  rc=%d !\n", fs_id, rc);
		exit (1);
	}

	parent = KVSNS_ROOT_INODE;
	rc = kvsns_creat(fs_ctx, &cred, &parent, "fichier", 0755, &ino);
	if (rc != 0) {
		if (rc == -EEXIST)
			fprintf(stderr, "dirent exists\n");
		else {
			fprintf(stderr, "kvsns_creat: err=%d\n", rc);
			exit(1);
		}
	}

	/* kvsns_open is not implemeted yet */
#if 0
	rc = kvsns2_open(fs_ctx, &cred, &ino, 0 /* to raise errors laters */,
			 0755, &fd);
	if (rc != 0) {
		fprintf(stderr, "kvsns_open: err=%d\n", rc);
		exit(1);
	}
#else
	assert(0);
#endif

	strncpy(buff, "This is the content of the file", SIZE);
	count = strnlen(buff, SIZE);
	offset = 0;

	/* TODO: Update this call to include filesystem context */
	written = kvsns2_write(fs_ctx, &cred, &fd, buff, count, offset);
	if (written < 0) {
		fprintf(stderr, "kvsns_write: err=%lld\n",
			(long long)written);
		exit(1);
	}

	read = kvsns2_read(fs_ctx, &cred, &fd, read_buff, count, offset);
	if (read < 0) {
		fprintf(stderr, "kvsns_read: err=%lld\n",
			(long long)read);
		exit(1);
	}

	if (read == written) {
		printf("Buffer data: %s\n", read_buff);
	}
	else {
		fprintf(stderr, "Read and write size does not match\n");
		exit(1);
	}

#if 0
	rc = kvsns2_close(fs_ctx, &fd);
	if (rc != 0) {
		fprintf(stderr, "kvsns_close: err=%d\n", rc);
		exit(1);
	}
#endif

	printf("######## OK ########\n");
	return 0;

}
