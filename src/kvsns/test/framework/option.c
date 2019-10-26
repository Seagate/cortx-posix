/**
 * @file option.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  command line argument parser utility to.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for O_DIRECTORY and O_DIRECT */
#endif

#include <stdio.h>
#include <string.h>
#include <error.h>
#include <option.h>

int parse_user_cred(char *str_user_cred, kvsns_cred_t *cred)
{
	int rc = 0;
	char *tmp;

	cred->uid = 0;
	cred->gid = 0;

	tmp = strtok(str_user_cred, ":|");
	cred->uid = atoi(tmp);

	tmp = strtok(NULL, ":|");
	cred->gid = atoi(tmp);

	tmp = strtok(NULL, ":|");
	if (tmp) {
		fprintf(stderr, "too many user credentials : %s\n", tmp);
		rc = -1;
		goto error;
	}

	if (cred->uid <= 0)
		cred->uid = getuid();

	if (cred->gid <= 0)
		cred->uid = getuid();

error:
	return rc;
}

mode_map_t mode_table[] = {
	{"S_IRWXU", S_IRWXU},
	{"S_IRUSR", S_IRUSR},
	{"S_IWUSR", S_IWUSR},
	{"S_IXUSR", S_IXUSR},
	{"S_IRWXG", S_IRWXG},
	{"S_IRGRP", S_IRGRP},
	{"S_IWGRP", S_IWGRP},
	{"S_IXGRP", S_IXGRP},
	{"S_IRWXO", S_IRWXO},
	{"S_IROTH", S_IROTH},
	{"S_IWOTH", S_IWOTH},
	{"S_IXOTH", S_IXOTH},
	{NULL, 0} 
};

int parse_mode(char *str_mode, mode_t *return_mode)
{
	int rc = 0;
	mode_t mode = 0;
	char *tmp = NULL;

	*return_mode = 0;

	if (str_mode == NULL) {
		fprintf(stderr, "Insufficient memory.\n");
		rc = -1;
		goto error;
	}

	mode = strtol(str_mode, NULL, 8);
	if (mode !=  INT_MIN && mode != INT_MAX && mode != 0) {
#ifdef DEBUG
		printf("mode = %d\n",mode);
#endif
		goto out;
	} else 
		mode = 0;

	for (tmp = strtok(str_mode, ":|"); tmp;
			tmp = strtok(NULL, ":|")) {
		int i = 0;
#ifdef DEBUG
		printf("mode = %s\n",tmp);
#endif
		for (i = 0; mode_table[i].name != NULL; i++) {
			if (!strcmp(tmp, mode_table[i].name)){
				mode |= mode_table[i].mode;
				break;
			}
		}

		if (mode_table[i].name == NULL) {
			fprintf(stderr, "No such mode: %s\n",
					tmp);
			rc = -1;
			goto error;
		}
	}

#ifdef DEBUG
	printf("mode = %d\n", mode);
#endif

out:
	*return_mode = mode;
error:
	return rc;
}

flag_map_t flag_table[] = {
	{"O_RDONLY", O_RDONLY},
	{"O_WRONLY", O_WRONLY},
	{"O_RDWR", O_RDWR},
	{"O_CREAT", O_CREAT},
	{"O_CLOEXEC", O_CLOEXEC},
	{"O_EXCL", O_EXCL},
	{"O_NOCTTY", O_NOCTTY},
	{"O_TRUNC", O_TRUNC},
	{"O_APPEND", O_APPEND},
	{"O_NONBLOCK", O_NONBLOCK},
	{"O_NDELAY", O_NDELAY},
	{"O_SYNC", O_SYNC},
#ifdef O_DIRECT
	{"O_DIRECT", O_DIRECT},
#endif
	{"O_LARGEFILE", O_LARGEFILE},
	{"O_DIRECTORY", O_DIRECTORY},
	{"O_NOFOLLOW", O_NOFOLLOW},
	{NULL, 0}
};

int parse_flags(char *str_flags, int *return_flags)
{
	int rc = 0;
	int flags = 0;
	char *tmp = NULL;

	*return_flags = 0;

	if (str_flags == NULL) {
		fprintf(stderr, "Insufficient memory.\n");
		rc = -1;
		goto error;
	}

	flags = atoi(str_flags);
	if (flags > 0) {
#ifdef DEBUG
		printf("flags = %d\n",flags);
#endif
		goto out;
	} else 
		flags = 0;

	for (tmp = strtok(str_flags, ":|"); tmp;
			tmp = strtok(NULL, ":|")) {
		int i = 0;
#ifdef DEBUG
		printf("flags = %s\n",tmp);
#endif
		for (i = 0; flag_table[i].name != NULL; i++) {
			if (!strcmp(tmp, flag_table[i].name)){
				flags |= flag_table[i].flag;
				break;
			}
		}

		if (flag_table[i].name == NULL) {
			fprintf(stderr, "No such flag: %s\n",
					tmp);
			rc = -1;
			goto error;
		}
	}

#ifdef DEBUG
	printf("flags = %x\n", flags);
#endif
out:
	*return_flags = flags;
error:
	return rc;
}

int parse_io_mode(char *str_read_mode, int *read_mode)
{
	int rc = STATUS_OK;
	
	if (!strcmp(str_read_mode, STR_IO_MODE_FILE)) {
		*read_mode = IO_MODE_FILE;
	} else if (!strcmp(str_read_mode, STR_IO_MODE_INODE)) {
		*read_mode = IO_MODE_INODE;
	} else {
		fprintf(stderr, "Invalid read mode : %s", str_read_mode);
		rc = STATUS_FAIL;
	}

	return rc;
}
