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
#include <sys/syscall.h>
#include <libgen.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>


static int setattr(void *ctx, kvsns_ino_t *ino, char *name, char *val,
		   struct stat *stats)
{
	int rc = 0;
	int flag;
	long int attr;
	kvsns_cred_t cred;
	char *endptr;

	cred.uid = getuid();
	cred.gid = getgid();

	if (!strcmp(name, "gid")) {
		attr = strtol(val, &endptr, 10);
		fprintf(stderr, "ino=%llu gid=%ld\n", *ino, attr);
		stats->st_gid = (gid_t)attr;
		flag = STAT_GID_SET;
	}
	else if (!strcmp(name, "uid")) {
		attr = strtol(val, &endptr, 10);
		fprintf(stderr, "ino = %llu uid = %ld\n", *ino, attr);
		stats->st_uid = (uid_t)attr;
		flag = STAT_UID_SET;
	}
	else if (!strcmp(name, "atime")) {
		attr = strtol(val, &endptr, 10);
		fprintf(stderr, "ino = %llu atime = %ld\n", *ino, attr);
		stats->st_atim.tv_sec = (__time_t)attr;
		flag = STAT_ATIME_SET;
	}
	else if (!strcmp(name, "mtime")) {
		attr = strtol(val, &endptr, 10);
		fprintf(stderr, "ino = %llu mtime = %ld\n", *ino, attr);
		stats->st_mtim.tv_sec = (__time_t)attr;
		flag = STAT_MTIME_SET;
	}
	else if (!strcmp(name, "ctime")) {
		attr = strtol(val, &endptr, 10);
		fprintf(stderr, "ino = %llu ctime: %ld\n", *ino, attr);
		stats->st_ctim.tv_sec = (__time_t)attr;
		flag = STAT_CTIME_SET;
	}
	else {
		fprintf(stderr, "ino = %llu Invalid attribute name: %s\n",
			*ino, name);
		rc = -EINVAL;
		goto out;
	}

	rc = kvsns2_setattr(ctx, &cred, ino, stats, flag);
	if (rc == 0)
		fprintf(stderr, "setattr %llu/%s %d OK\n", *ino,
			 name, atoi(val));
	else
		fprintf(stderr, "Can't setattr %llu/%s rc=%d\n",
			*ino, name, rc);

out:
	return rc;
}

static int kvsns_str2ino(const char *str, kvsns_ino_t *ino)
{
	unsigned long long val;
	char *end;
	int rc;

	if (strnlen(str, 256) > snprintf(NULL, 0, "%llu", ULLONG_MAX)) {
		fprintf(stderr, "String %s is too long\n", str);
		rc = -EINVAL;
		goto out;
	}

	val = strtoull(str, &end, 0);
	if (val == ULLONG_MAX || val == 0) {
		if (errno == ERANGE) {
			fprintf(stderr, "ERANGE while parsing ino %s\n", str);
			rc = -EINVAL;
			goto out;
		}
	}

	if (end != (str + strlen(str))) {
		fprintf(stderr, "Trailing non-numeric chars (%s) "
			"at the end of %s\n", end, str);
		rc = -EINVAL;
		goto out;
	}

	*ino = val;
	rc = 0;

out:
	return rc;
}

static int kvsns_rename_pre(const kvsns_cred_t *pcred,
			    uint64_t fs_id, kvsns_fs_ctx_t *fs_ctx)
{
	return kvsns_create_fs_ctx(fs_id, fs_ctx);
}

static int kvsns_rename_cmd(kvsns_fs_ctx_t fs_ctx, const kvsns_cred_t *pcred,
			    const char *s_src_dir_ino,
			    const char *s_src_name,
			    const char *s_dst_dir_ino,
			    const char *s_dst_name)
{
	int rc;
	kvsns_ino_t sdir_ino;
	kvsns_ino_t ddir_ino;
	kvsns_cred_t cred;
	static const kvsns_cred_t root_cred = { .uid = 0, .gid = 0 };

	rc = kvsns_str2ino(s_src_dir_ino, &sdir_ino);
	if (rc != 0) {
		fprintf(stderr, "Cannot parse <src_dir>\n");
		goto out;
	}
	rc = kvsns_str2ino(s_dst_dir_ino, &ddir_ino);
	if (rc != 0) {
		fprintf(stderr, "Cannot parse <dst_dir>\n");
		goto out;
	}

	if (pcred) {
		cred = *pcred;
	} else {
		cred = root_cred;
	}

	rc = kvsns_rename(fs_ctx, &cred,
			  &sdir_ino, (char *) s_src_name,
			  &ddir_ino, (char *) s_dst_name);
out:
	return rc;
}

struct readdir_ctx {
	int index;
};

static bool readdir_cb(void *ctx, const char *name, const kvsns_ino_t *ino)
{
	struct readdir_ctx *readdir_ctx = ctx;

	printf("READDIR:\t\"%d\" \"%s\" \"%llu\"\n",
	       readdir_ctx->index, name, *ino);

	readdir_ctx->index++;

	return true;
}

static int kvsns_readdir_cmd(kvsns_fs_ctx_t fs_ctx, const kvsns_cred_t *cred,
			const char *s_ino)
{
	int rc;
	kvsns_ino_t ino;
	struct readdir_ctx readdir_ctx[1] = {{
		.index = 0,
	}};

	rc = kvsns_str2ino(s_ino, &ino);
	if (rc != 0) {
		fprintf(stderr, "Cannot parse <s_ino>");
		goto out;
	}

	printf("READDIR_BEGIN: (%llu) INDEX -> NAME -> INODE \n", ino);
	rc = kvsns_readdir(fs_ctx, cred, &ino, readdir_cb, readdir_ctx);
	printf("READDIR_END: %llu\n", ino);

out:
	return rc;
}

int main(int argc, char *argv[])
{
	int rc;
	kvsns_cred_t cred;
	kvsns_file_open_t kfd;

	char current_path[MAXPATHLEN];
	char prev_path[MAXPATHLEN];
	char exec_name[MAXPATHLEN];
	char k[KLEN];
	char v[VLEN];

	kvsns_ino_t current_inode = 0LL;
	kvsns_ino_t parent_inode = 0LL;
	kvsns_ino_t ino = 0LL;
	uint64_t fs_id = 1;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;

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

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu,  rc=%d !\n", fs_id, rc);
			exit (1);
		}

		rc = kvsns2_creat(fs_ctx, &cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "kvsns_open")) {
		if (argc != 2) {
			fprintf(stderr, "open <filename>\n");
			exit(1);
		}
		/* @todo: kvsns_lookup_path needs to be updated to use filesystem context */
		rc = kvsns_lookup_path(&cred, &current_inode, argv[1], &ino);
		if (rc != 0) {
			fprintf(stderr, "kvsns_lookup_path: err=%d\n", rc);
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu,  rc=%d !\n", fs_id, rc);
			exit(1);
		}

		rc = kvsns2_open(fs_ctx, &cred, &ino, O_RDONLY, 0644, &kfd);
		if (rc == 0) {
			printf("Current thread id is %u \n", (unsigned int) syscall( __NR_gettid ));
			printf("==> %llu/%s opened by pid = %d and tid = %d\n",
			       current_inode, argv[1], kfd.owner.pid, kfd.owner.tid);
		} else
			printf("Failed rc=%d !\n", rc);

		rc = kvsns2_close(fs_ctx, &kfd);
		if (rc == 0) {
			printf("Current thread id is %u \n", (unsigned int) syscall( __NR_gettid ));
			printf("==> %llu/%s closed for pid = %d and tid = %d\n",
			       current_inode, argv[1], kfd.owner.pid, kfd.owner.tid);
			return 0;
		} else
			printf("Close Failed rc=%d !\n", rc);


	} else if (!strcmp(exec_name, "kvsns_lookup")) {

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu,  rc=%d !\n", fs_id, rc);
			exit(1);
		}
		if (argc == 2) {
			rc = kvsns2_lookup(fs_ctx, &cred, &current_inode, argv[1], &ino);
			if (rc == 0) {
				printf("LOOKUP: %llu/%s = %llu\n",
				current_inode, argv[1], ino);
				return rc;
			} else
				printf("==> %llu/%s No such file or directory, rc=%d!\n",
				       current_inode, argv[1],rc);
		} else if (argc == 3) {
			kvsns_ino_t pino;
			rc = kvsns_str2ino(argv[1], &pino);
			if (rc != 0) {
				printf("Invalid inode.\n");
				exit(1);
			}
			rc = kvsns2_lookup(fs_ctx, &cred, &pino, argv[2], &ino);
			if (rc == 0) {
				printf("LOOKUP: %llu/%s = %llu\n",
				pino, argv[1], ino);
				return rc;
			} else
				printf("==> %llu/%s No such file or directory, rc= %d!\n",
				       pino, argv[1] ,rc);

		}
		else {
			fprintf(stderr, "kvsns_lookup  <filename_in_root_dir> OR \n");
			fprintf(stderr, "kvsns_lookup  <parent inode> <filename> \n");
			exit(1);
		}

	} else if (!strcmp(exec_name, "kvsns_del")) {
		if (argc != 2) {
			fprintf(stderr, "kvsns_del <filename>\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
			exit(1);
		}

		rc = kvsns2_unlink(fs_ctx, &cred, &current_inode, argv[1]);
		if (rc == 0) {
			printf("==> Deleted %llu/%s \n", current_inode, argv[1]);
			return rc;
		} else
			printf("==> Delete failed %llu/%s, rc: %d!\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "kvsns_getattr")) {
		struct stat buffstat;

		if (argc != 2) {
			fprintf(stderr, "getattr <name>\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
			exit(1);
		}

		if (!strcmp(argv[1], "."))
			ino = current_inode;
		else {
			rc = kvsns2_lookup(fs_ctx, &cred, &current_inode, argv[1], &ino);
			if (rc != 0)
				return rc;
		}

		rc = kvsns2_getattr(fs_ctx, &cred, &ino, &buffstat);
		if (rc == 0) {
			printf(" inode: %ld\n", buffstat.st_ino);
			printf(" mode: %o\n", buffstat.st_mode);
			printf(" number of hard links: %d\n",
			       (int)buffstat.st_nlink);
			printf(" user ID of owner: %d\n", buffstat.st_uid);
			printf(" group ID of owner: %d\n", buffstat.st_gid);
			printf(" total size, in bytes: %ld\n",
			       buffstat.st_size);
			printf(" blocksize for filesystem I/O: %ld\n",
				buffstat.st_blksize);
			printf(" number of blocks allocated: %ld\n",
				buffstat.st_blocks);
			printf(" time of last access: %ld : %s",
				buffstat.st_atime,
				ctime(&buffstat.st_atime));
			printf(" time of last modification: %ld : %s",
				 buffstat.st_mtime,
				 ctime(&buffstat.st_mtime));
			printf(" time of last change: %ld : %s",
				buffstat.st_ctime,
				 ctime(&buffstat.st_ctime));

			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
		return 0;
	} else if (!strcmp(exec_name, "kvsns_setattr")) {
		kvsns_ino_t fino = 0LL;
		struct stat stat;
		memset(&stat, 0, sizeof(stat));

		if (argc != 4) {
			fprintf(stderr, "Failed!\n");
			fprintf(stderr, "kvsns_setattr  <file> <attr_name> <value>\n");
			fprintf(stderr, "supported attr_name: uid | gid | atime | mtime | ctime\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
			exit(1);
		}
		rc = kvsns2_lookup(fs_ctx, &cred, &current_inode, argv[1], &fino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			exit(1);
		}
		rc = setattr(fs_ctx, &fino, argv[2], argv[3], &stat);
		if (rc != 0) {
			fprintf(stderr, "setting %s : %s failed, rc=%d\n",
			        argv[3], argv[2], rc);
		}
	} else if (!strcmp(exec_name, "kvsns_mkdir")) {
		if (argc != 2) {
			fprintf(stderr, "kvsns_dir <dirname>\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
			exit(1);
		}

		rc = kvsns_mkdir(fs_ctx, &cred, &current_inode, argv[1], 0755, &ino);
		if (rc == 0) {
			printf("==> Created dir %llu/%s \n", current_inode, argv[1]);
			return rc;
		} else
			printf("==> mkdir failed %llu/%s, rc: %d!\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "kvsns2_mkdir")) {
		if (argc != 3) {
			fprintf(stderr, "kvsns2_mkdir <parent_ino> <dirname>\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
			exit(1);
		}

		kvsns_ino_t parent_ino;
		rc = kvsns_str2ino(argv[1], &parent_ino);
		if (rc != 0) {
			printf("Invalid inode.\n");
			exit(1);
		}
		rc = kvsns_mkdir(fs_ctx, &cred, &parent_ino, argv[2], 0755, &ino);
		if (rc == 0) {
			printf("==> Created dir %llu/ (%s) \n", parent_ino, argv[2]);
			return rc;
		} else
			printf("==> mkdir failed %llu/%s, rc: %d!\n",
				current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "kvsns_rmdir")) {
		if (argc != 3) {
			fprintf(stderr, "kvsns_rmdir <parent_ino> <dirname>\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			printf("Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
			exit(1);
		}

		kvsns_ino_t parent_ino;
		rc = kvsns_str2ino(argv[1], &parent_ino);
		if (rc != 0) {
			printf("Invalid inode.\n");
			exit(1);
		}
		rc = kvsns2_rmdir(fs_ctx, &cred, &parent_ino, argv[2]);
		if (rc == 0) {
			printf("==> Deleted dir %llu/%llu (%s) \n", parent_ino,
			       ino, argv[2]);
			return rc;
		} else
			printf("==> rmdir failed %llu/ (%s), rc=%d!\n",
			       parent_ino, argv[2], rc);

	} else if (!strcmp(exec_name, "kvsns_rename")) {
		if (argc != 5) {
			fprintf(stderr, "kvsns_rename <sino> <src> <dino> <dst>\n");
			exit(1);
		}
		rc = kvsns_rename_pre(&cred, fs_id, &fs_ctx);
		if (rc != 0) {
			fprintf(stderr, "Failed to prepare env for rename\n");
			exit(1);
		}
		rc = kvsns_rename_cmd(fs_ctx, &cred, argv[1], argv[2], argv[3], argv[4]);
		if (rc == 0) {
			printf("==> Renamed %llu/%s/%s to %llu/%s/%s \n",
			       current_inode, argv[1], argv[2],
			       current_inode, argv[3], argv[4]);
			return rc;
		} else
			printf("==> Renamed failed %llu/%s/%s to %llu/%s/%s, rc = %d \n",
			       current_inode, argv[1], argv[2],
			       current_inode, argv[3], argv[4], rc);
	} else if (!strcmp(exec_name, "kvsns_readdir")) {
		if (argc != 2) {
			fprintf(stderr, "kvsns_readdir <ino>\n");
			exit(1);
		}
		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			fprintf(stderr, "Failed to prepare env for readdir\n");
			exit(1);
		}
		rc = kvsns_readdir_cmd(fs_ctx, &cred, argv[1]);
		if (rc == 0) {
			printf("==> READDIR ok for %llu/%s\n",
			       current_inode, argv[1]);
			return rc;
		} else
			printf("==> READDIR failed %llu/%s, rc = %d \n",
			       current_inode, argv[1], rc);
	} else if (!strcmp(exec_name, "kvsns_ln")) {
		if (argc != 3) {
			fprintf(stderr, "ln <newdir> <content>\n");
			exit(1);
		}
		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			fprintf(stderr, "Failed to prepare env for symlink\n");
			exit(1);
		}
		rc = kvsns_symlink(fs_ctx, &cred, &current_inode, argv[1],
				    argv[2], &ino);
		if (rc == 0) {
			printf("==> %llu/%s created = %llu\n",
				current_inode, argv[1], ino);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "kvsns_readlink")) {
		char link_content[MAXPATHLEN];
		size_t size = MAXPATHLEN;

		if (argc != 2) {
			fprintf(stderr, "readlink <link>\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			fprintf(stderr, "Failed to prepare env for readlink\n");
			exit(1);
		}
		rc = kvsns2_lookup(fs_ctx, &cred, &current_inode, argv[1], &ino);
		if (rc != 0) {
			printf("==> %llun not accessible = %llu : rc=%d\n",
				current_inode, ino, rc);
			return 0;
		}

		rc = kvsns_readlink(fs_ctx, &cred, &ino,
				    link_content, &size);
		if (rc == 0) {
			printf("==> %llu/%s %llu content = %s\n",
				current_inode, argv[1], ino, link_content);
			return 0;
		} else
			printf("Failed rc=%d !\n", rc);
	} else if (!strcmp(exec_name, "kvsns_link")) {
		kvsns_ino_t dino = 0LL;
		kvsns_ino_t sino = 0LL;
		char dname[MAXNAMLEN];

		if (argc != 3 && argc != 4) {
			printf("ns_link srcname newname (same dir)\n");
			printf("ns_link srcname dstdir newname\n");
			exit(1);
		}

		rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
		if (rc != 0) {
			fprintf(stderr, "Failed to prepare env for readlink\n");
			exit(1);
		}

		rc = kvsns2_lookup(fs_ctx, &cred, &current_inode, argv[1], &sino);
		if (rc != 0) {
			fprintf(stderr, "%s/%s does not exist\n",
				current_path, argv[1]);
			exit(1);
		}

		if (argc == 3) {
			dino = current_inode;
			strncpy(dname, argv[2], MAXNAMLEN);
		} else if (argc == 4) {
		  rc = kvsns2_lookup(fs_ctx, &cred, &current_inode,
				  argv[2], &dino);
			if (rc != 0) {
				fprintf(stderr, "%s/%s does not exist\n",
					current_path, argv[1]);
				exit(1);
			}
			strncpy(dname, argv[3], MAXNAMLEN);
		}
		printf("%s: sino=%llu dino=%llu dname=%s\n",
			exec_name, sino, dino, dname);

		rc = kvsns_link(fs_ctx, &cred, &sino, &dino, dname);
		if (rc == 0)
			printf("ns_link: %llu --> %llu/%s CREATED\n",
				sino, dino, dname);
		else
			fprintf(stderr, "Failed : %d\n", rc);
	} else
		fprintf(stderr, "%s does not exists\n", exec_name);
	printf("######## OK ########\n");
	return rc;
}
