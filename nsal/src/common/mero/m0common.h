#ifndef _M0COMMON_H
#define _M0COMMON_H

/* -*- C -*- */
/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 *       * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <fnmatch.h>
#include <ini_config.h>

#include <kvstore.h>
#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"

#ifdef DEBUG
#define M0_DASSERT(cond) assert(cond)
#else
#define M0_DASSERT(cond)
#endif

typedef bool (*get_list_cb)(char *k, void *arg);

int m0init(struct collection_item *cfg_items);
void m0fini(void);
int m0kvs_reinit(void);

int m0kvs_set(void *ctx, void *k, const size_t klen,
	       void *v, const size_t vlen);
int m0kvs4_set(void *k, const size_t klen,
	       void *v, const size_t vlen);
int m0kvs_get(void *ctx, void *k, size_t klen, void **v, size_t *vlen);
int m0kvs4_get(void *k, size_t klen, void **v, size_t *vlen);
int m0kvs_del(void *ctx, void *k, const size_t klen);
void m0_iter_kvs(char *k);
int m0_pattern_kvs(void *ctx, char *k, char *pattern,
		    get_list_cb cb, void *arg_cb);
int m0_key_prefix_exists(void *ctx, const void *kprefix, size_t klen,
			 bool *result);
int m0store_create_object(struct m0_uint128 id);
int m0store_delete_object(struct m0_uint128 id);
int m0_ufid_get(struct m0_uint128 *ufid);
int m0_fid_to_string(struct m0_uint128 *fid, char *fid_s);
void *m0kvs_alloc(uint64_t size);
void m0kvs_free(void *ptr);

/******************************************************************************/
/* Key Iterator */

/* TODO:PERF:
 *	Performance of key iterators can be improved by:
 *	1. Usage of prefetch.
 *	2. Async Clovis calls.
 *	3. Piggyback data for records.
 *	The features can be implemented without significant changes in
 *	the caller code and mostly isolated in the m0common module.
 *
 *	1. The key prefetch feature requires an additional argument to specify
 *	the amount of records to retrieve in a NEXT clovis call.
 *	Then, key_iter_next walks over the prefetched records and issues
 *	a NEXT call after the last portion of records was processed by the user.
 *
 *	2. The async clovis calls feature can be used to speed up the case
 *	where the time of records processing by the caller is comparable with
 *	the time needed to receive next bunch of records from Clovis.
 *	In this case a initial next call synchronously gets a bunch of records,
 *	and then immediately issues an asynchronous NEXT call.
 *	The consequent next call waits for the issued records,
 *	and again issues a NEXT call to clovis. In conjunction with the prefetch
 *	feature, it can help to speed up readdir() by issuing NEXT (dentry)
 *	and GET (inode attrs) concurrently.
 *
 *	3. Since readdir requires a combination of NEXT + GET calls,
 *	the iterator can issue a GET call to get the inode attirbutes of
 *	prefetched dentries along with the next portion of NEXT dentries.
 *	So that, we can get a chunk of dentries and the attributes of the
 *	previous chunck witin a single clovis call.
 *	However, this feature make sense only for the recent version of
 *	nfs-ganesha where a FSAL is resposible for filling in attrlist
 *	(the current version calls fsal_getattr() instead of it).
 */

/** Find the first record following by the prefix and set iter to it.
 * @param iter Iterator object to initialized with the starting record.
 * @param prefix Key prefix to be found.
 * @param prefix_len Length of the prefix.
 * @return True if found, otherwise False. @see kvstore_iter::inner_rc for return
 * code.
 */
bool m0_key_iter_find(struct kvstore_iter *iter, const void* prefix,
		      size_t prefix_len);

/** Cleanup key iterator object */
void m0_key_iter_fini(struct kvstore_iter *iter);

/* Find the next record and set iter to it. */
bool m0_key_iter_next(struct kvstore_iter *iter);

size_t m0_key_iter_get_key(struct kvstore_iter *iter, void **buf);
size_t m0_key_iter_get_value(struct kvstore_iter *iter, void **buf);

/******************************************************************************/

#define M0STORE_BLK_COUNT 10

enum io_type {
	IO_READ = 1,
	IO_WRITE = 2
};

#define CONF_STR_LEN 100

ssize_t m0store_get_bsize(struct m0_uint128 id);

ssize_t m0store_do_io(struct m0_uint128 id, enum io_type iotype, off_t x,
		      size_t len, size_t bs, char *buff);

/* Deallocates space allocated for `fid` in the range specified by
 * `count` and `offset` -- the first deallocated byte is fid[offset]
 * and the last deallocated byte is fid[offset + count - 1].
 * If the range is not properly aligned with the block size of the
 * underlying storage then either block space is filled with zeros (1) or
 * block is deleted (2), or both (1+2).
 *
 *  1.     | block1 | block2 | <- no deallocations
 *	      |00000| <- added zeros instead of data
 *
 *  2.     | block1 | block2 | < block2 is deallocated
 *                  |   |
 *
 *  1+2.   | block1 | block2 | <- block2 is deallocated
 *            |00000    | <- AND zeros instead of data for block1
 */
int m0_file_unmap(struct m0_uint128 fid, size_t count, off_t offset);

static inline ssize_t m0store_pwrite(struct m0_uint128 id, off_t x,
				     size_t len, size_t bs, char *buff)
{
	return m0store_do_io(id, IO_WRITE, x, len, bs, buff);
}

static inline ssize_t m0store_pread(struct m0_uint128 id, off_t x,
				    size_t len, size_t bs, char *buff)
{
	return m0store_do_io(id, IO_READ, x, len, bs, buff);
}

static inline void m0_fid_copy(struct m0_uint128 *src, struct m0_uint128 *dest)
{
	dest->u_hi = src->u_hi;
	dest->u_lo = src->u_lo;
}

int m0_idx_create(struct m0_clovis_idx **index);

/** Creates a new index (a container for KVS records) in the underlying storage (Mero)
 * and returns a handle to be used in KVS operations within this index.
 * Note: The caller should release the resources associated with the handle
 * by calling m0idx_close.
 * @param fid In FID 
 * @param index Out on success a valid initialized clovis index is returned.
 */
int m0idx_create(const struct m0_uint128 *fid, struct m0_clovis_idx **index);

/** Delete clovis index instance.
 * @param fid In FID 
 * @param index Out on success a valid initialized clovis index is returned.
 */
int m0idx_delete(const struct m0_uint128 *fid);

/** Initialize clovis index instance.
 * @param fid In FID 
 * @param index Out on success a valid initialized clovis index is returned.
 */
int m0idx_open(const struct m0_uint128 *fid, struct m0_clovis_idx **index);

/** Finalize clovis index instance.
 * @param index Index to be finalized.
 */
void m0idx_close(struct m0_clovis_idx *index);

/* To be removed by Amit's work */
char * m0_get_gfid();

#endif
/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
