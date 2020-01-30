/*
 * Filename:         dstore.h
 * Description:      data store module of DSAL

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains dstore framework infrastructure.
 There are currently 1 users of this infrastructure,
        1. eos

 This gives capability of data storing.
*/

#ifndef _DSTORE_H
#define _DSTORE_H

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint*_t */
#include <stdbool.h> /* bool */
#include <sys/types.h> /* off_t */
#include <sys/stat.h> /* stat */
#include <object.h> /* obj_id_t */

struct collection_item;

typedef obj_id_t dstore_oid_t;

struct dstore;

int dstore_init(struct collection_item *cfg, int flags);

int dstore_fini(struct dstore *dstore);

/*
 * @todo: http://gitlab.mero.colo.seagate.com/eos/fs/dsal/issues/2
 */
int dstore_obj_create(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid);

int dstore_obj_delete(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid);

int dstore_obj_read(struct dstore *dstore, void *ctx,
		    dstore_oid_t *oid, off_t offset,
		    size_t buffer_size, void *buffer, bool *end_of_file,
		    struct stat *stat);

int dstore_obj_write(struct dstore *dstore, void *ctx,
		     dstore_oid_t *oid, off_t offset,
		     size_t buffer_size, void *buffer, bool *fsal_stable,
		     struct stat *stat);

int dstore_obj_resize(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid,
		      size_t old_size, size_t new_size);

int dstore_get_new_objid(struct dstore *dstore, dstore_oid_t *oid);

struct dstore_ops {
	/* dstore module init/fini */
	int (*init) (struct collection_item *cfg);
	int (*fini) (void);

	/* Object operations */
	int (*obj_create) (struct dstore *dstore, void *ctx,
			   dstore_oid_t *oid);
	int (*obj_delete) (struct dstore *dstore, void *ctx,
			   dstore_oid_t *oid);
	int (*obj_read) (struct dstore *dstore, void *ctx,
			 dstore_oid_t *oid, off_t offset,
		   	 size_t buffer_size, void *buffer, bool *end_of_file,
		   	 struct stat *stat);
	int (*obj_write) (struct dstore *dstore, void *ctx,
			  dstore_oid_t *oid, off_t offset,
			  size_t buffer_size,
		    	  void *buffer, bool *fsal_stable, struct stat *stat);
	int (*obj_resize) (struct dstore *dstore, void *ctx,
			   dstore_oid_t *oid,
		           size_t old_size, size_t new_size);
	int (*obj_get_id) (struct dstore *dstore, dstore_oid_t *oid);
};

struct dstore *dstore_get(void);

#endif
