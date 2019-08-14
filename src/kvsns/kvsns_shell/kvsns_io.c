opyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Vishal Tripathi
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

/* kvsns_io.c
 * KVSNS: does to @ files to/from KVSNS
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

#define IOLEN 4096

static void exit_rc(char *msg, int rc)
{
	if (rc >= 0)
		return;

	char format[MAXPATHLEN];

	snprintf(format, MAXPATHLEN, "%s%s",
		 msg, " |rc=%d\n");

	fprintf(stderr, format, rc);
	exit(1);
}

int main(int argc, char *argv[])
{
	kvsns_cred_t cred;
	kvsns_ino_t parent;
	kvsns_ino_t ino;
	kvsns_file_open_t kfd;
	int fd = 0;
	bool kvsns_src = false;
	bool kvsns_dest = false;
	char *src = NULL;
	char *dest = NULL;
	int rc;

	if (argc != 2) {
		fprintf(stderr, "%s <src> \n", argv[0]);
		exit(1);
	}

	cred.uid = getuid();
	cred.gid = getgid();

	if (!strncmp(argv[1], KVSNS_URL, KVSNS_URL_LEN)) {
		kvsns_src = true;
		src = argv[1] + KVSNS_URL_LEN;
	}

	printf("%s, %u \n", src, kvsns_src);

	if (!kvsns_src)
		fd = open(src, O_RDONLY);

	if (fd < 0) {
		fprintf(stderr, "Can't open POSIX fd, errno=%u\n", errno);
		exit(1);
	}

	/* Start KVSNS Lib */
	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	exit_rc("kvsns_start faild", rc);

	rc = kvsns_get_root(&parent);
	exit_rc("Can't get KVSNS's root inode", rc);

	if (kvsns_src) {
		rc = kvsns_lookup_path(&cred, &parent, src, &ino);
		exit_rc("Can't lookup dest in KVSNS", rc);
		rc = kvsns_open(&cred, &ino, O_RDONLY, 0644, &kfd);
	}

	exit_rc("Can't open file in KVSNS", rc);

	/* Deal with the copy */
	if (kvsns_src)
		rc = kvsns_cp_from(&cred, &kfd, fd, IOLEN);

	exit_rc("Copy failed", rc);

	/* The end */
	rc = close(fd);
	exit_rc("Can't close POSIX fd, errno=%d", errno);

	rc = kvsns_close(&kfd);
	exit_rc("Can't close KVSNS fd, errno=%d", errno);

	return 0;
}

