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

/** Base data type that represents a data-store object in open state. */
struct dstore_obj;

/** Base data type that represents an IO operation. */
struct dstore_io_op;

/* see the description in dstore_bufvec.h */
struct dstore_io_vec;

int dstore_init(struct collection_item *cfg, int flags);

int dstore_fini(struct dstore *dstore);

/*
 * @todo: http://gitlab.mero.colo.seagate.com/eos/fs/dsal/issues/2
 */
int dstore_obj_create(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid);

int dstore_obj_delete(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid);

/* Deprecated: should be removed when dstore_io_op_read is fully implemented. */
int dstore_obj_read(struct dstore *dstore, void *ctx,
		    dstore_oid_t *oid, off_t offset,
		    size_t buffer_size, void *buffer, bool *end_of_file,
		    struct stat *stat);

/* Deprecated: should be removed when dstore_io_op_write is fully implemented. */
int dstore_obj_write(struct dstore *dstore, void *ctx,
		     dstore_oid_t *oid, off_t offset,
		     size_t buffer_size, void *buffer, bool *fsal_stable,
		     struct stat *stat);

/* TODO: Should be replaced by dstore_io_op_deallocate?
 * XXX: Also, the name is not correct: the function deallocates
 * space but does not perform any changes of the object size.
 */
int dstore_obj_resize(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid,
		      size_t old_size, size_t new_size);

int dstore_get_new_objid(struct dstore *dstore, dstore_oid_t *oid);

/** Prepare object for IO operation.
 * This function prepares a data-store object for IO operations.
 * It may involve internal IO (for example to fetch the object
 * layout, to check object existence etc) and/or syscalls
 * (for example, POSIX open or POSIX read).
 * @param[in]  oid Object ID (u128) of object in "dstore".
 * @param[out] out In-memory representation of an open object.
 * @return 0 or -errno.
 */
int dstore_obj_open(struct dstore *dstore,
		    const dstore_oid_t *oid,
		    struct dstore_obj **out);

/** Release resources associated with an open object.
 * This function de-allocates memory and any other sources taken by
 * dstore_obj_open call.
 * The function blocks on a call to the unerlying storage until
 * all the in-flight IO operations are stable. This behavior provides
 * open-to-close consistency.
 * An attempt to close already closed object causes undefined
 * behavior (double-free).
 * @param[in] obj Open object.
 * @return 0 or -errno.
 */
int dstore_obj_close(struct dstore_obj *obj);

/** A user-provided callback to receive notifications. */
typedef void (*dstore_io_op_cb_t)(void *cb_ctx,
				  struct dstore_io_op *op,
				  int op_rc);

/** Initialize, submit and return WRITE IO operation.
 * This function creates a new IO operation, submits it
 * for execution and then returns it to the caller.
 * The IO operation created by this call should be finalized
 * using dstore_io_op_fini.
 * @param[in] obj An open object.
 * @param[in] bvec Associated data buffers and offsets to be written.
 * @param[in, opt] cb Callback for notifications. Can be NULL.
 * @param[in, opt] cb_ctx User-provided context for the callback. Can be NULL.
 * @param[out] out A new IO operation created by this function.
 * @return 0 or -errno.
 */
int dstore_io_op_write(struct dstore_obj *obj,
		       struct dstore_io_vec *bvec,
		       dstore_io_op_cb_t cb,
		       void *cb_ctx,
		       struct dstore_io_op **out);

/** Initialize, submit and return READ IO operation.
 * This function creates a new IO operation, submits it
 * for execution and then returns it to the caller.
 * The IO operation created by this call should be finalized
 * using dstore_io_op_fini.
 * @param[in] obj An open object.
 * @param[in] bvec Associated data buffers and offsets to be read out.
 * @param[in, opt] cb Callback for notifications. Can be NULL.
 * @param[in, opt] cb_ctx User-provided context for the callback. Can be NULL.
 * @param[out] out A new IO operation created by this function.
 * @return 0 or -errno.
 */
int dstore_io_op_read(struct dstore_obj *obj,
		      struct dstore_io_vec *bvec,
		      dstore_io_op_cb_t cb,
		      void *cb_ctx,
		      struct dstore_io_op **out);

/** Wait until the IO operation is stable. */
int dstore_io_op_wait(struct dstore_io_op *op);

/** Release the resources associated with an IO operation. */
int dstore_io_op_fini(struct dstore_io_op *op);

struct dstore *dstore_get(void);

#endif
