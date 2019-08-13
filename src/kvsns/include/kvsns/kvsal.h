/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
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

/* kvsns_test.c
 * KVSNS: simple test
 */

#ifndef _KVSAL_H
#define _KVSAL_H

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/xattr.h>
#include <ini_config.h>

#include <kvsns/common.h>

#define KVSAL_ARRAY_SIZE 100

#define KVSNS_STORE "KVSNS_STORE"
#define KVSNS_SERVER "KVSNS_SERVER"
#define KLEN 256
#define VLEN 256


int kvsal_init(struct collection_item *cfg_items);
int kvsal_fini(void);
int kvsal_begin_transaction(void);
int kvsal_end_transaction(void);
int kvsal_discard_transaction(void);
int kvsal2_exists(void * ctx, char *k, size_t klen);
int kvsal2_set_char(void *ctx, char *k, size_t klen, char *v, size_t vlen);
int kvsal2_set_bin(void *ctx, const void *k, size_t klen, const void *v,
		   size_t vlen);
int kvsal2_get_char(void *ctx, char *k, size_t klen, char *v, size_t vlen);
int kvsal2_get_bin(void *ctx, const void *k, size_t klen, void *v, size_t vlen);
int kvsal3_get_bin(void *ctx, void *k, const size_t klen, void **v, size_t *vlen);
int kvsal3_set_bin(void *ctx, void *k, const size_t klen, void *v,
		   const size_t vlen);
int kvsal2_del(void *ctx, char *k, size_t klen);
int kvsal2_del_bin(void *ctx, const void *key, size_t klen);
int kvsal2_incr_counter(void *ctx, char *k, unsigned long long *v);

int kvsal_create_fs_ctx(unsigned long fs_id, void **fs_ctx);

#if 1
int kvsal_set_char(char *k, char *v);
int kvsal_get_char(char *k, char *v);
int kvsal_set_binary(char *k, char *buf, size_t size);
int kvsal_get_binary(char *k, char *buf, size_t *size);
int kvsal_set_stat(char *k, struct stat *buf);
int kvsal_get_stat(char *k, struct stat *buf);
int kvsal_del(char *k);
#endif

/******************************************************************************/
/* Index Mgmt */
int kvsal_create_fs_ctx(unsigned long fs_id, void **fs_ctx);

/* TODO:Resource mgmt: Add kvsal_destroy_fs_ctx here */

/* TODO:Multiple indexes: Add kvsal_index_open and kvsal_index_close here */

/******************************************************************************/
/* KV-records memory mgmt */
int kvsal_alloc(void **ptr, uint64_t size);
void kvsal_free(void *ptr);

/******************************************************************************/
/* Key iterator API */

/** Max size of implementation-defined data for a kval_iter. */
#define KVSAL_ITER_PRIV_DATA_SIZE 64

/** An iterator object for walking over KVS records in the specified index. */
struct kvsal_iter {
	/** Index context */
	void *ctx;
	/** Return code of the last find/next call. */
	int inner_rc;
	/** Implementation-define private data. */
	char priv[KVSAL_ITER_PRIV_DATA_SIZE];
};

/** An iterator for walking over records with the same key prefix. */
struct kvsal_prefix_iter {
	struct kvsal_iter base;
	/** Pointer to prefix buffer owned by callers. */
	const void *prefix;
	/** Size of the prefix. */
	size_t prefix_len;
};

/** Find first record with the specified prefix.
 * @return True if the start record found.
 */
bool kvsal_prefix_iter_find(struct kvsal_prefix_iter *iter);

/** Find the record follwing by the current record.
 * @return True if the next record found.
 */
bool kvsal_prefix_iter_next(struct kvsal_prefix_iter *iter);

/** Free resources allocated by find/next calls. */
void kvsal_prefix_iter_fini(struct kvsal_prefix_iter *iter);

/** Get pointer to key data.
 * @param[out] buf View of key data owned by iter.
 * @return Size of key.
 */
size_t kvsal_iter_get_key(struct kvsal_iter *iter, void **buf);

/** Get pointer to value data.
 * @param[out] buf View of value data owned by iter.
 * @return Size of value.
 */
size_t kvsal_iter_get_value(struct kvsal_iter *iter, void **buf);

/******************************************************************************/
#endif
