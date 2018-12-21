#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ini_config.h>

#include <kvsns/extstore.h>

#define BUFFSIZE 40960

static struct collection_item *cfg_items;
#define CONF "~/kvsns.ini"

int copy_to_mero(int fd_source, kvsns_ino_t *ino,
		 int iolen, size_t filesize)
{
	off_t off;
	size_t rsize, wsize;
	int remains;
	size_t len;
	char buff[BUFFSIZE];
	bool stable;
	struct stat stat;

	remains = filesize;
	off = 0LL;
	while (off < filesize) {
		len = (remains > iolen) ? iolen : remains;

		rsize = pread(fd_source, buff, len, off);
		if (rsize < 0)
			return -1;

		wsize = extstore_write(ino, off, rsize, buff, &stable, &stat);
		if (wsize < 0)
			return -1;

		if (wsize != rsize)
			return -1;

		if (wsize < 0)
			return -1;

		if (wsize != rsize)
			return -1;

		off += rsize;
		remains -= rsize;
	}

	/* Think about setting MD */
	return 0;
}

int main(int argc, char *argv[])
{
	int fds;
	kvsns_ino_t ino;
	struct stat stat;
	int iolen;
	int rc;
	struct collection_item *errors = NULL;

	if (argc != 4) {
		fprintf(stderr, "cp <src> <id> <iolen>\n");
		exit(1);
	}

	rc = config_from_file("libkvsns", CONF, &cfg_items,
				INI_STOP_ON_ERROR, &errors);
	if (rc) {
		fprintf(stderr, "Can't load config rc=%d\n", rc);

		free_ini_config_errors(errors);
		return -rc;
	}

	rc = extstore_init(cfg_items);
	if (rc != 0) {
		fprintf(stderr, "Can't start extstore lib, rc=%d\n", rc);
		exit(1);
	}

	iolen = atoi(argv[3]);
	printf("Start: iolen=%u\n", iolen);

	ino = atoi(argv[2]);

	fds = open(argv[1], O_RDONLY);
	if (fds < 0) {
		fprintf(stderr, "Can't open %s : %d\n",
			argv[1], errno);
		exit(1);
	}

	if (fstat(fds, &stat) < 0) {
		fprintf(stderr, "Can't stat %s: %d\n", argv[1], errno);
		exit(1);
	}

	rc = extstore_create(ino);
	if (rc != 0) {
		fprintf(stderr, "Can't create object rc=%d\n", rc);
		exit(1);
	}

	/* Must read and write on fd */
	if (copy_to_mero(fds, &ino, iolen, stat.st_size) < 0) {
		fprintf(stderr, "Can't copy to %s to MERO\n",
			argv[1]);
		exit(1);
	}

	close(fds);

	return 0;
}
