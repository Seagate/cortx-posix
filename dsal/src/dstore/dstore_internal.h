/*
 * Filename:         dstore_internal.h
 * Description:      Private DSAL API.
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
 */

/* This file defines an API that is used and implemented by DSAL
 * backends ("plugins").
 * Each backend must implement the vtable (dstore_ops) and correspondingly
 * extend the basic data types used by the vtable (ops and objects).
 * Note: This is a private API and it should not be visible to users.
 */
#ifndef _DSTORE_INTERNAL_H
#define _DSTORE_INTERNAL_H

#include "dstore.h" /* import public data types */

struct dstore_ops;
static inline
bool dstore_ops_invariant(const struct dstore_ops *ops);

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

static inline
bool dstore_invariant(const struct dstore *dstore)
{
	/* Condition:
	 *	Dstore backend operations should be valid.
	 */
	bool ops_are_valid = dstore_ops_invariant(dstore->dstore_ops);

	return ops_are_valid;
}


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
 * In other words, the base data type should always be the first field
 * in the user-defined structure. It helps to avoid unnecessary
 * "container_of" calls, and allows us to use typecasts directly.
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

	/** Cached Object ID value.
	 * The backend-defined information will likely have a duplicate
	 * of this value, however, in order to get the ID we would need
	 * to have an indirect call to the backend. So there is a trade-off
	 * between an indirect call and extra 16 bytes per dstore_obj
	 * instance. At this point, it is unknown what is better - an
	 * extra indirect call (affects CPU) or extra 16 bytes (affects
	 * memory consumption). Moreover, a backend that does not
	 * keep track of it (for example POSIX-based) may not have this value
	 * stored inside private data. Considering these points,
	 * right now the best solution is to add this field and hide it with
	 * help of a getter (see ::dstore_obj_id).
	 */
	obj_id_t oid;
	/** Beginning of backend-defined information. */
	uint8_t priv[0];
};

static inline
bool dstore_obj_invariant(const struct dstore_obj *obj)
{
	/* Condition:
	 *	Object should have a reference to dstore.
	 *	The reference should point to a valid dstore object.
	 */
	return obj->ds != NULL && dstore_invariant(obj->ds);
}

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
	uint8_t *buf;
	uint64_t size;
	uint64_t offset;
};

static inline
bool dstore_io_buf_invariant(const struct dstore_io_buf *io_buf)
{
	/* Condition:
	 *	Non-empty buffer should have "size" field set.
	 */
	bool should_have_size_if_has_data = ((!io_buf->buf) || io_buf->size);
	return should_have_size_if_has_data;
}

/** An immutable (no append, no remove) vector of data buffers.
 * The IO vector comprises
 *	- well-known scatter-gather fields for
 *	  keeping pointers to array of data buffers;
 *	- (unused right now) flags to modify behavior;
 *	- embedded data buffer to reduce allocations of intermediate
 *	  objects for single-buffer IO (which is the only use case right now).
 * NOTE: This structure was deliberately set to be opaque and to have an
 * internal embedded buffer because of the conflicting requirements that
 * exist at this moment:
 *	- API should support vectored IO;
 *	- API should be consistent;
 *	- API should not expose backend-dependent data types;
 *	- DSAL should always have common data types;
 *	- The amount of memory allocations should be reduced as much
 *	  as possible.
 *	- memcpy (even for small objects) should be avoided.
 *	- DSAL should be optimized for the current workload (non-vectored IO).
 *	- API should be simple and easy-to-use.
 * Considering these points, the essential parts of IO interface (data vector
 * and data buffer) were made to be opaque for the callers.
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

	/* Embedded data buffer: an in-place storage for
	 * non-vectored IO operations.
	 * It helps to reduce unnecessary memory
	 * allocations for operations that work on
	 * a single extent.
	 */
	struct dstore_io_buf edbuf;
};

static inline
bool dstore_io_vec_invariant(const struct dstore_io_vec *io_vec)
{
	/* Condition:
	 *	If vector has elements then the corresponding fields
	 *	should be filled.
	 * NOTE: This condition may be changed in future if
	 * we need to implement Alloc/Free operations.
	 */
	bool non_empty_vec_has_data = ((!io_vec->nr) ||
				       (io_vec->dbufs && io_vec->svec &&
					io_vec->ovec && io_vec->bsize));
	/* Condition:
	 *	The embedded buffer should always be in the right state
	 *	(whether it is empty or not).
	 */
	bool embed_is_valid = dstore_io_buf_invariant(&io_vec->edbuf);
	return non_empty_vec_has_data && embed_is_valid;
}

/** Check if io_vec is just a single buffer embedded into io_vec itself. */
static inline
bool dstore_io_vec_is_embed(const struct dstore_io_vec *v)
{
	return v->edbuf.buf != NULL && v->dbufs == &v->edbuf.buf;
}

/** An utility function to wire embedded buffer with the "interface"
 * pointers. Before calling this function, the vector may
 * (or rather should) violate its invariant (because dbufs and the other
 * things are not wired together). After calling this function, the invariant
 * should be held (as long as the embedded buffer is valid).
 * Therefore, this function is highly unsafe and it can be used only at
 * initialization steps.
 */
static inline
void dstore_io_vec_set_from_edbuf(struct dstore_io_vec *v)
{
	v->dbufs = &v->edbuf.buf;
	v->svec = &v->edbuf.size;
	v->ovec = &v->edbuf.offset;
	v->bsize = v->edbuf.size;
	v->nr = 1;
}

/** Moves io_vec value from one object into another. */
static inline
void dstore_io_vec_move(struct dstore_io_vec *dst, struct dstore_io_vec *src)
{
	/* copy embedded buffer state */
	dst->edbuf = src->edbuf;

	/* update refs for embedded case */
	if (dstore_io_vec_is_embed(src)) {
		dstore_io_vec_set_from_edbuf(dst);
	}

	/* nullify the source */
	*src = (struct dstore_io_vec) { .nr = 0 };
}


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
 * should extend the structure if it requires additional
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
	/** Optional callback to notify the user about
	 * operation state changes.
	 */
	dstore_io_op_cb_t cb;
	/** Optional context for the callback. */
	void *cb_ctx;

	/** Beginning of backend-defined information. */
	uint8_t priv[0];
};

static inline
bool dstore_io_op_invariant(const struct dstore_io_op *op)
{
	/* Condition:
	 *	Op type should be a supported operation.
	 * NOTE: only WRITE is supported so far.
	 */
	bool op_is_supported = (op->type == DSTORE_IO_OP_WRITE);
	/* Condition:
	 *	Data vector should be a valid object whether it has
	 *	data or not.
	 */
	bool has_valid_vec = dstore_io_vec_invariant(&op->data);
	/* Condition:
	 *	Operation always holds a borrowed reference to
	 *	the associated object.
	 */
	bool has_ref_to_obj = op->obj != NULL;

	/* Condition:
	 *	IO operations require non-empty vectors.
	 * NOTE: This condition should be changed/removed
	 *	 when Alloc/Free implemented.
	 */
	bool has_needed_data = (op->data.nr != 0);

	return op_is_supported && has_valid_vec && has_needed_data &&
		has_ref_to_obj;
}

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

	/** DSAL.OP_FINI Interface.
	 * This function releases all the resources take by
	 * the corresponding IO operation.
	 * Note: This function will block (if it is necessary)
	 * to ensure the operation is safely removed/canceled
	 * from any queues. The caller is responsible for waiting
	 * until stable/failed state (using io_op_submit or the callback
	 * mechanism) to achieve better performance. However, it is known that
	 * the operation will not block (for example, if it is already
	 * finished with success/failure) then this call won't affect
	 * performance.
	 */
	void (*io_op_fini)(struct dstore_io_op *op);

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

static inline
bool dstore_ops_invariant(const struct dstore_ops *ops)
{
	/* Condition:
	 *	All callbacks should be available except those
	 *	which are not implemented (or not in use right now).
	 */
	return
		ops->init &&
		ops->fini &&
		ops->obj_create &&
		ops->obj_delete &&
		ops->obj_read &&
		ops->obj_write &&
		ops->obj_resize &&
		ops->obj_get_id &&
		ops->obj_open &&
		ops->obj_close &&
		ops->io_op_init &&
		ops->io_op_submit &&
		ops->io_op_wait &&

		/* AllocBuf/FreeBuf interfaces are not in use right now. */
#if 0
		ops->alloc_buf &&
		ops->free_buf &&
#endif
		true;
}

/* FIXME: This structure does not belong here.
 * It should be moved into eos-related directory.
 * The consumer (dstore.c) should include this file separately.
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

static inline
const obj_id_t *dstore_obj_id(const struct dstore_obj *obj)
{
	return &obj->oid;
}

#endif
