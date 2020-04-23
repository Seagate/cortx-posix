/*
 * Filename:         dstore_bufvec.h
 * Description:      IO Vectors and buffers.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 */
#ifndef DSTORE_BUFVEC_H_
#define DSTORE_BUFVEC_H_
/******************************************************************************/
/******************************************************************************/

/* Opaque data type that represents a single datum for an IO operation.
 * The data type holds the following information:
 *	- offset in the object.
 *	- IO size.
 *	- IO data buffer.
 * This data type is used in non-vectored IO operations similiar
 * to pwrite/pread.
 */
struct dstore_io_buf;

/* Create a new buffer. */
int dstore_io_buf_init(void *data, size_t size, size_t offset,
		      struct dstore_io_buf **buf);

/* Release resources associated with the buffer. */
int dstore_io_buf_fini(struct dstore_io_buf *buf);

/** Get a pointed to the data buffer. */
const void *dstore_io_buf_data(const struct dstore_io_buf *buf);

/** Opaque data type that represents a group of buffers.
 * The data type is used in vectored (scatter-gather) IO operations
 * similar to pwritev/preadv.
 * The vector itself is immutable, i.e. users cannot
 * append new data buffers or remove data buffers from it.
 */
struct dstore_io_vec;

/** Create a new vector with the specified size. */
int dstore_io_vec_init(size_t count,
		       struct dstore_io_vec **vec);

/* Get the size of a vector. */
size_t dstore_io_vec_count(struct dstore_io_vec *vec);

/** Zerocopy conversion from a single buffer.
 * Moves data from a single buffer into a vector of buffers.
 * Note: this call consumes "buf" and replaces the value
 * with NULL.
 */
int dstore_io_buf2vec(struct dstore_io_buf **buf, struct dstore_io_vec **vec);

/** Zerocopy conversion into a single buffer.
 * Moves data from a vector with size 1 into a buffer.
 * If the vector size is diffrent from 1, the function
 * returns EINVAL.
 */
int dstore_io_vec2buf(struct dstore_io_vec **buf, struct dstore_io_buf **vec);

/* Set data for existing element of a vector (zerocopy).
 * NOTE: consumes the provided buffer.
 */
int dstore_io_vec_set(struct dstore_io_vec *vec,
		      size_t index, struct dstore_io_buf **buf);

/** Get data from existing element of a vector (zerocopy).
 * NOTE: consumes internal buffer associated with the index,
 * and returns it up to the caller.
 */
int dstore_io_vec_get(const struct dstore_io_vec *vec,
		      size_t index, struct dstore_io_buf **buf);

/* Release resources allocated for the buf vector */
void dstore_io_vec_fini(struct dstore_io_vec *vec);

/******************************************************************************/
#endif /* DSTORE_BUFVEC_H_ */
