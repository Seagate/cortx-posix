/*
 * Filename:         dstore_internal.h
 * Description:      Private DSAL API.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 *
 * This file defines an API that is used and implemented by DSAL
 * backends ("plugins").
 * Each backend must implement the vtable (dstore_ops) and correspondingly
 * extend the basic data types used by the vtable (ops and objects).
 * Note: This is a private API and it should not be visible to users.
 */
#ifndef _DSTORE_INTERNAL_H
#define _DSTORE_INTERNAL_H

#include "dstore.h" /* import public data types */

struct dstore {
	/* Type of dstore, currently eos supported */
	char *type;
	/* Config for the dstore specified type */
	struct collection_item *cfg;
	/* Operations supported by dstore */
	const struct dstore_ops *dstore_ops;
	/* Not used currently */
	int flags;
};

/** A base object for DSAL operations.
 *
 * The memory layout that all DSAL backends should
 * support:
 * @{verbtim}
 * +------------------------------------------------+
 * | dstore_obj created by obj_open()               |
 * +-------------------+----------------------------+
 * | struct dstore_obj | implementation-defined     |
 * |                   | fields that keep private   |
 * |                   | data                       |
 * +-------------------+----------------------------+
 * @{endverbatim}
 * In other workds, the base data type should always be the first field
 * in the user-defined structure. It helps to avoid unnessesary
 * "container_of" calls, and let us use typecasts directly.
 *
 * Example:
 * @{code}
 *	struct foo_dstore_obj {
 *		struct dstore_obj base;
 *		struct foo foo;
 *	};
 *	int foo_dstore_open(..., struct dstore_obj **out) {
 *	  struct foo_dstore_obj *obj;
 *	  // ...
 *	  obj = alloc(...);
 *	  // ...
 *	  *out = &obj->base;
 *	  return rc;
 *	}
 *	int foo_dstore_close(struct dstore_obj *base) {
 *	  struct foo_dstore_obj *obj;
 *	  obj = (struct foo_dstore_obj *) base;
 *	}
 * @{endcode}
 */
struct dstore_obj {
	/** A weak (borrowed) reference to the dstore
	 * where this object was open.
	 */
	struct dstore *ds;
	/** Beginning of backend-defined information. */
	uint8_t priv[0];
};

/** A single IO buffer.
 * This structure has two primary use cases:
 *	- creating a single IO datum without having troubles
 *	  with io_vec initialization. See the buf2vec
 *	  and vec2buf functions in the public API.
 *	- appending data buffer into io_vec.
 * Note: the later case is considered to-be-implemented
 * because right now io_vec is immutable (the nr field
 * should not be modified after vector is created).
 */
struct dstore_io_buf {
	void *buf;
	uint64_t size;
	uint64_t offset;
};

/** An immutable (no append, no remove) vector of data buffers.
 */
struct dstore_io_vec {
	/* Array of pointers to data buffers. */
	uint8_t **dbufs;
	/** Array of data buffer sizes. */
	uint64_t *svec;
	/** Array of offsets. */
	uint64_t *ovec;
	/** Number of elements in the arrays. */
	uint64_t nr;
	/** Controls alignment-related invariants. */
	uint64_t bsize;

	/* To-be-defined:
	 * Various flags for changing policies and
	 * behavior of io_vec. For example,
	 * to indicate ownership status of data buffers.
	 */
	uint64_t flags;
};

/** IO operations available for a datastore object. */
enum dstore_io_op_type {
	/** Pre-allocates (reserves) storage space. */
	DSTORE_IO_OP_ALLOC,
	/** De-allocates storage space (TRIM/UNMAP). */
	DSTORE_IO_OP_FREE,
	/** Modifies regions of an object. */
	DSTORE_IO_OP_WRITE,
	/** Reads regions of an object. */
	DSTORE_IO_OP_READ,
};

/** Base data type for IO operations.
 * Memory layout is the same as for dstore_obj - a backend
 * should extend the structure if it requres additional
 * fields.
 */
struct dstore_io_op {
	/** IO type */
	enum dstore_io_op_type type;
	/** A weak (non-owning) pointer to the object used
	 * for this IO operation.
	 */
	struct dstore_obj *obj;
	/** Data buffers and offsets associated with the operation.
	 * The IO operation object owns all indirect objects including
	 * the data buffers.
	 * TODO: io_vec can be updated to have a flag to indicate
	 *       borrowed data. The corresponding constructor
	 *       may be added to the public API.
	 */
	struct dstore_io_vec data;
	/** Optional callback to nofity the user about
	 * operation state changes.
	 */
	dstore_io_op_cb_t cb;
	/** Optional context for the callback. */
	void *cb_ctx;

	/** Beginning of backend-defined information. */
	uint8_t priv[0];
};

/** Vtable for DSAL backends.
 * This structure describes a list of functions that
 * should be implemented by every DSAL backend.
 * NOTE: This is a private interface not exposed to
 * the user.
 */
struct dstore_ops {
	/* dstore module init/fini */
	int (*init) (struct collection_item *cfg);
	int (*fini) (void);

	/* TODO: "ctx" should be removed from the interface
	 * because it carries no useful information.
	 * TODO: This function does not have well-defined behavior
	 * yet. This needs to be addressed.
	 */
	int (*obj_create) (struct dstore *dstore, void *ctx,
			   dstore_oid_t *oid);

	/* TODO: "ctx" should be removed from the interface
	 * because it carries no useful information.
	 * TODO: This function does not have well-defined behavior
	 * yet. This needs to be addressed.
	 */
	int (*obj_delete) (struct dstore *dstore, void *ctx,
			   dstore_oid_t *oid);

	/* FIXME: Deprecated interface */
	int (*obj_read) (struct dstore *dstore, void *ctx,
			 dstore_oid_t *oid, off_t offset,
			 size_t buffer_size, void *buffer, bool *end_of_file,
			 struct stat *stat);

	/* FIXME: Deprecated interface */
	int (*obj_write) (struct dstore *dstore, void *ctx,
			  dstore_oid_t *oid, off_t offset,
			  size_t buffer_size,
			  void *buffer, bool *fsal_stable, struct stat *stat);

	/* FIXME: Deprecated interface */
	int (*obj_resize) (struct dstore *dstore, void *ctx,
			   dstore_oid_t *oid,
		           size_t old_size, size_t new_size);

	/*
	 * TODO: This function does not have well-defined behavior
	 * yet. This needs to be addressed.
	 */
	int (*obj_get_id) (struct dstore *dstore, dstore_oid_t *oid);

	/* DSAL.OPEN Interface.
	 * This function should create a new in-memory representation
	 * of the object associated with the given ID.
	 * The function provides no guarantees regarding
	 * uniqueness of the returned in-memory object.
	 * However, the function must ensure to return an object
	 * ready for IO. For example, if the object does not exist
	 * then -ENOENT should be returned. If the backend
	 * supports "Create-on-Write" semantic then
	 * the obj_create function must ensure to write data
	 * into the object, so that DSAL.OPEN will always
	 * return an existing object.
	 */
	int (*obj_open)(struct dstore *dstore,
			const dstore_oid_t *oid,
			struct dstore_obj **obj);

	/* DSAL.CLOSE Interface.
	 * This function should release all resources associated
	 * with the given object.
	 * DSAL does not track on-going operations. If DSAL.CLOSE
	 * is called for an object that has unfinished IO then
	 * it may lead to corruption (basically, behavior is undefined).
	 */
	int (*obj_close)(struct dstore_obj *obj);

	/* DSAL.OP_INIT Interface.
	 * This function creates a new IO operation using the
	 * given inputs.
	 */
	int (*io_op_init)(struct dstore_obj *obj,
			  enum dstore_io_op_type type,
			  struct dstore_io_vec *bvec,
			  dstore_io_op_cb_t cb,
			  void *cb_ctx,
			  struct dstore_io_op **out);

	/* DSAL.OP_SUBMIT Interface.
	 * This function sends an IO operation to be executed.
	 * The function can be noop for some of the backends
	 * if they don't support this feature.
	 */
	int (*io_op_submit)(struct dstore_io_op *op);

	/* DSAL.OP_WAIT Interface.
	 * This function blocks on waiting for operation to
	 * become stable. The user must ensure to call
	 * this function when the completion callback
	 * (cb+cb_ctx) is not used.
	 */
	int (*io_op_wait)(struct dstore_io_op *op);

	/* DSAL.ALLOC_BUF
	 * This function allocates a memory region aligned
	 * with the required boundaries.
	 * The function is optional, and may be replaced
	 * by calloc if the backend does not have any
	 * specific requirements regarding memory alignment
	 * of data buffers.
	 */
	int (*alloc_buf)(struct dstore *, void **out);

	/* DSAL.FREE_BUF
	 * This function releases a memory region taken by DSAL.ALLOC_BUF.
	 * The function should support the semantic of free(NULL)
	 * (free(NULL) is noop).
	 */
	void (*free_buf)(struct dstore *, void *);
};

/* FIXME: This structure does not belong here.
 * It should be moved into eos-related directory.
 * The consumer (dstore.c) should include this file separtely.
 * It will help to remove dependencies between DSAL backends.
 */
extern const struct dstore_ops eos_dstore_ops;


/** A helper for DSTORE backends: initializes already-allocated
 * IO operation using the given arguments.
 * This function can be used to reduce boilerplate code in
 * implementation of DSAL.OP_INIT interface.
 */
void dstore_io_op_init(struct dstore_obj *obj,
		       enum dstore_io_op_type type,
		       struct dstore_io_vec *bvec,
		       dstore_io_op_cb_t cb,
		       void *cb_ctx,
		       struct dstore_io_op *op);

#endif
