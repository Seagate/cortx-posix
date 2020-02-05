#ifndef DSAL_IO_H_
#define DSAL_IO_H_
/******************************************************************************/
/* TODO: Add Copyright. */
/******************************************************************************/
/* Data Types */

/** Global context for the data storage module.
 * It is the root object for the whole module.
 */
struct dsal;

/** Per-os (object storage) context.
 * In case if a DSAL backend supports logical separation (containers) for objects
 * then this is a separate object. Otherwise, it can simply point to the
 * global dsal context.
 */
struct dsal_os;

/** An object in OPEN state.
 * Depending on the backend implementation, it can be a POSIX file descriptor
 * or a runtime object (Clovis) or simply an FID (no runtime states).
 */
struct dsal_obj;

/** A user-supplied callback to check the object state.
 * DSAL does not hold any runtime containers for object
 * states. Instead, the caller provides a callback that looks up the fid
 * in its cache for any associated states with the given object.
 */
typedef bool (*dsal_is_open_ucb_t)(struct dsal_os *, struct dsal_fid_t *);

struct dsal_user_cbs {
	dsal_is_open_ucb_t is_open;
};

/******************************************************************************/
/* Methods */

int dsal_init(void *config, struct dsal_user_cbs *user_cbs);

/** Initialize an object store.
 * It might be a noop for some backends.
 */
int dsal_os_init(struct dsal *dsal, struct dsal_os **os);

/** Close an object store context */
int dsal_os_fini(struct dsal_os *os);

/** Open an object in an object store. */
int dsal_object_open(struct dsal_os *os, const dsal_fid_t *fid,
		     struct dsal_obj *obj);

/** Close an open object */
int dsal_object_close(struct dsal_obj *obj);

/** Create a new object in the store. */
int dsal_object_create(struct dsal_os *os, const dsal_fid_t *fid,
		       struct dsal_obj *obj);

/** Delete object from the store */
int dsal_object_delete(struct dsal_os *os, const dsal_fid_t *fid);

/** De-allocate space allocated for the object. */
int dsal_object_trim(struct dsal_os *os,
		     const dsal_fid_t *fid,
		     uint64_t count,
		     uint64_t offset);

/** Returns FID of an object. */
const dsal_fid_t *dsal_obj_get_fid(struct dsal_obj *obj);



/******************************************************************************/
/* Compatibility layer with the old API:
 *	- sync write,
 *	- sync read,
 *	- get new fid.
 */

/** Synchronous write operation for an object. */
int dsal_object_write(struct dsal_os *os, const dsal_fid_t *fid,
		      const void *data, size_t count, off_t offset);

/** Synchronous read operation for an object. */
int dsal_object_read(struct dsal_os *os, const dsal_fid_t *fid,
		     void *buf, size_t count, off_t offset);

int dsal_get_new_fid(struct dsal_os *os, dsal_fid_t *fid);

/******************************************************************************/
#endif /* DSAL_IO_H_ */
