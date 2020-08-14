/*
 * Filename: dsal_async_io.h
 * Description: Async IO module for Data storage abstraction layer
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

#ifndef DSAL_ASYNC_IO_H_
#define DSAL_ASYNC_IO_H_

/** Async IO module for Data storage abstraction layer.
 * The module API allows users to submit a single IO operation into the
 * underlying data storage and get a notification (success/failure) in
 * a user-defined callback/callbacks.
 *
 * Threading
 * ---------
 * User-defined callbacks are called from DSAL-backend-owned threads.
 * It means it is up to the user to pass this information to the right
 * consumer.
 *
 * Memory
 * ------
 * The module provides types for IO operations to ensure that the caller
 * does not need to use heap allocations for each IO operation.
 * However, it introduces another problem - an ABI between the modules.
 * In order to make sure that, this won't cause undefined behavior on the
 * DSAL side, the init function checks the size.
 *
 * Usage
 * -------------
 *
 * @{code}
 * // A structure on the caller side to be used
 * // as a user-defined context for AIO operations.
 * // In this example we simply use it to count
 * // the amount of ongoing IO operations.
 * struct fd_state {
 *	struct dsal_obj *obj;
 *	// Number of active IO operations.
 *	atomic_int_t n_inflight
 * };
 *
 * void dsal_write_cb(void *ctx, struct dsal_op *op, int rc)
 * {
 *   struct fd_state *state = ctx;
 *   state->n_inflight--;
 *   // dsal_op is destroyed by dsal aio module.
 * }
 *
 * int dsal_write(struct fd_state *state, void *data, off_t offset, size_t count)
 * {
 *   struct dsal_aio_op op;
 *   int rc;
 *   rc = dsal_obj_create_aio_op(state->obj, &op, dsal_write_cb, state);
 *   if (rc != 0) {
 *     return rc;
 *   }
 *   rc = dsal_aio_op_write(&op, data, offset, count);
 *   if (rc != 0) {
 *      dsal_aio_op_fini(&op);
 *      return rc;
 *   }
 *   rc = dsal_aio_op_submit(&op);
 *   if (rc != 0) {
 *      dsal_aio_op_fini(&op);
 *      return rc;
 *   }
 *   state->n_inflight++;
 *   return rc;
 * }
 * @{endcode}
 */
/******************************************************************************/
/* Data types */

/* Max size of a storage for a dsal_aio_op. */
#define DSAL_AIO_PRIV_SIZE 256

/** DSAL AIO Operation.
 * Note: the operation is allocated on the callers stack,
 * initialized using the dsal_obj_create_aio_op
 * and freed by the AIO module after returning from the
 * callback. In case if the caller wants to deallocate
 * the resources that the operation hold, it should
 * use dsal_aio_op_fini call.
 */
struct dsal_aio_op {
	char priv[DSAL_AIO_PRIV_SIZE];
};

/** A callback to be called on aio op completion.
 * @param cb_ctx - The caller context passed as cb_ctx in the init operation.
 * @param op - The completed operation.
 * @param rc - The return code of the operation: 0 (succ) or -errno (fail).
 */
typedef void (*dsal_aio_op_cb_t)(void *cb_ctx, struct dsal_aio_op *op, int rc);

/******************************************************************************/
/* Methods */

/** Initialize DSAL AIO module.
 * @param dsal - A pointer to the global DSAL context.
 * @param dsal_io_io_size - The DSAL_AIO_PRIV_SIZE defined on the caller side.
 * @return 0 (succ) or -errno (fail).
 */
int dsal_aio_init(struct dsal *dsal, size_t dsal_aio_op_size);

/** Create a new AIO operation for the given object.
 * Note: This function creates an operation that is capable of holding
 * only one (data,count,offset) tuple. In other words,
 * multiple calls of aio_op_write REPLACE the tuple stored
 * in the operation structure.
 * For vectored IO, the caller should use dsal_obj_create_aio_vecop().
 * The reason to keep non-vectored separate is to allow the DSAL
 * implementation to optimize memory allocations.
 * It also removes the need to keep a complex logic for
 * unaligned or overlapping buffers. If the underlying store is not able
 * to handle unaligned (an explicit restriction for Mero OS) or
 * overlapping buffers (an implicit restriction for Mero OS), then
 * the DSAL implementation would need to use a complicated logic to handle this.
 */
int dsal_obj_create_aio_op(struct dsal_obj *obj,
			   struct dsal_aio_op *op,
			   dsal_io_op_cb_t cb,
			   void *cb_ctx);

int dsal_obj_create_aio_vecop(struct dsal_obj *obj,
			      struct dsal_io_op *op,
			      dsal_io_op_cb_t cb,
			      void *cb_ctx,
			      uint64_t nr_buffers);

/** Free resources allocated for an operation.
 * This function should be called by the API user in case if the user
 * wants to release the resources taken by an obj_create_aio_op() call.
 * It should not be called inside the user-defined completion callback
 * because DSAL frees the resources atomaticaly in this case.
 */
int dsal_aio_op_fini(struct dsal_aio_op *op);

/** Set data to be written within the operation.
 * Buffer ownership:
 *   The operation borrows the data pointer (as a immutable reference)
 * and keeps it until the operation is done (or finalized by user).
 * The caller should not modify the buffer while the operation is in progress.
 * Otherwise, it might cause undefined behavior in the underlying store.
 * Buffer size and offset:
 *   The operation works only with aligned buffers. See
 * ::dsal_obj_is_buffer_allowed_for_async for the details.
 */
int dsal_aio_op_write(struct dsal_io_op *op,
		      const void *data,
		      uint64_t count,
		      uint64_t offset);

/** Get a buffer to be filled with data read from the object
 * Buffer ownership:
 *   The operation requires a preallocated buffer with buf_size bytes.
 * It holds the data pointer as a mutable reference until the op is done
 * or finalized by user.
 * Buffer size and offset:
 *   The operation works only with aligned buffers. See
 * ::dsal_obj_is_buffer_allowed_for_async for the details.
 */
int dsal_aio_op_read(struct dsal_io_op *op,
		     void *buf,
		     uint64_t buf_size,
		     uint64_t offset);

/** Submit the IO operation to the underlying storage. */
int dsal_aio_op_submit(struct dsal_aio_op *op);


/** Checks if the buffer can be written into the object asynchronously.
 * The underlying store may have restrictions for data pointer alignment,
 * buffer size or offset. For example, a (size,size+offset) tuple should be
 * aligned with respect to PAGE_SHIFT or some inernal buffer or block size.
 * In this case, async IO is not allowed and the caller should
 * use the universal synchronous interface (dsal_object_read/write).
 * If the underlying store does not have such restrictions or async IO
 * implementaion is able to handle them, then this function is noop.
 */
bool dsal_obj_is_buffer_allowed_for_async(const struct dsal_obj *obj,
					  const void *buffer,
					  uint64_t size,
					  uint64_t offset);

/******************************************************************************/
#endif /* DSAL_ASYNC_IO_H_ */
