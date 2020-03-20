/*
 * Filename: efs_ut_helper.c
 * Description: Functions for fs_open and fs_close reuired for test
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Shraddha Shinde <shraddha.shinde@seagate.com>
 */

#include "efs_fs.h"

extern struct ut_efs_params ut_efs_obj;

int ut_efs_fs_setup()
{
	int rc = 0;

	ut_efs_obj.fs_ctx = EFS_NULL_FS_CTX;

	ut_efs_obj.cred.uid = getuid();
	ut_efs_obj.cred.gid = getgid();

	ut_efs_obj.current_inode = EFS_ROOT_INODE;
	ut_efs_obj.parent_inode = EFS_ROOT_INODE;

	rc = efs_init(EFS_DEFAULT_CONFIG);
	if (rc != 0) {
		fprintf(stderr, "efs_init: err = %d\n", rc);
		goto out;
	}

	char *name = "testfs";

	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));
	rc = 0;
	rc = efs_fs_create(&fs_name);

	if (rc != 0) {
		fprintf(stderr, "Failed to create FS %s, rc=%d\n", name, rc);
		goto out;
	}

	struct kvs_idx index;

	rc = efs_fs_open(name, &index);

	if (rc != 0) {
		fprintf(stderr, "Unable to open FS for fs_name:%s, rc=%d !\n",
			ut_efs_obj.fs_name, rc);
		goto out;
	}
	ut_efs_obj.fs_ctx = index.index_priv;

out:
	return rc;
}

int ut_efs_fs_teardown()
{
	efs_fs_close(ut_efs_obj.fs_ctx);

	char * name = "testfs";

	str256_t fs_name;
	str256_from_cstr(fs_name, name, strlen(name));

	efs_fs_delete(&fs_name);

	efs_fini();

	return 0;
}
