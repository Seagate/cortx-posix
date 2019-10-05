#include "test_ns_common.h"

void EOS_SETUP()
{
	int rc;
	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_start: err=%d\n", rc);
		exit(1);
	}
}

void EOS_TEARDOWN()
{
	int rc;
	rc = kvsns_stop();
        if (rc != 0) {
                fprintf(stderr, "kvsns_stop: err=%d\n", rc);
                exit(1);
        }
}

int KVSNS_MKDIR(char *name, mode_t mode)
{
	int rc;
	uint64_t fs_id = 1;
	kvsns_ino_t parent = 0LL;
	kvsns_ino_t ino = 0LL;
	kvsns_cred_t cred;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;

	cred.uid = getuid();
	cred.gid = getgid();
	rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
	if (rc != 0) {
		fprintf(stderr, "Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
		exit(1);
	}

	parent = KVSNS_ROOT_INODE;
	rc = kvsns_mkdir(fs_ctx, &cred, &parent, name, mode, &ino);
	return rc;
}

int KVSNS_RMDIR(char *name)
{
	int rc;
	uint64_t fs_id = 1;
	kvsns_ino_t parent = 0LL;
	kvsns_ino_t ino = 0LL;
	kvsns_cred_t cred;
	kvsns_fs_ctx_t fs_ctx = KVSNS_NULL_FS_CTX;

	rc = kvsns_create_fs_ctx(fs_id, &fs_ctx);
	if (rc != 0) {
		fprintf(stderr, "Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
		return rc;
	}

	parent = KVSNS_ROOT_INODE;
	rc  = kvsns2_lookup(fs_ctx, &cred, &parent, name, &ino);
	if (rc != 0) {
		fprintf(stderr, "kvsns_lookup: err=%d\n", rc);
		return rc;
	}

	rc = kvsns2_rmdir(fs_ctx, &cred, &parent, name);
		return rc;
}
