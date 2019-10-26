/**
 * @file eosfs.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  EOS utility functions.
 */

#include <stdio.h>
#include <eosfs.h>

/**
 * EOS start utility function.
 */
void eos_fs_init(void)
{
	int rc;
	rc = kvsns_start(KVSNS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "kvsns_start: err=%d\n", rc);
		exit(EXIT_FAILURE);
	}
}

/**
 * EOS stop utility function
 */
void eos_fs_fini(void)
{
	int rc;
	rc = kvsns_stop();
	if (rc != 0) {
		fprintf(stderr, "kvsns_stop: err=%d\n", rc);
		exit(EXIT_FAILURE);
	}
}

/**
 * EOS fs init utility function
 */
void eos_fs_open(uint64_t fs_id, kvsns_fs_ctx_t *fs_ctx)
{
	int rc;
	rc = kvsns_create_fs_ctx(fs_id, fs_ctx);
	if (rc != 0) {
		fprintf(stderr, "Unable to create index for fs_id:%lu, rc=%d \n", fs_id, rc);
		exit(EXIT_FAILURE);
	}
}
