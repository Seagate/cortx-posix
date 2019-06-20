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

typedef struct kvsal_item {
	int offset;
	char str[KLEN];
} kvsal_item_t;

typedef struct kvsal_list {
	char pattern[KLEN];
	kvsal_item_t *content;
	size_t size;
} kvsal_list_t;

int kvsal_init(struct collection_item *cfg_items);
int kvsal_fini(void);
int kvsal_begin_transaction(void);
int kvsal_end_transaction(void);
int kvsal_discard_transaction(void);
int kvsal_exists(char *k);
int kvsal2_exists(void * ctx, char *k, size_t klen);
int kvsal_set_char(char *k, char *v);
int kvsal2_set_char(void *ctx, char *k, size_t klen, char *v, size_t vlen);
int kvsal2_set_bin(void *ctx, const void *k, size_t klen, const void *v,
		   size_t vlen);
int kvsal_get_char(char *k, char *v);
int kvsal2_get_char(void *ctx, char *k, size_t klen, char *v, size_t vlen);
int kvsal2_get_bin(void *ctx, const void *k, size_t klen, void *v, size_t vlen);
int kvsal3_get_bin(void *ctx, void *k, size_t klen, void **v, size_t *vlen);
int kvsal3_set_bin(void *ctx, void *k, size_t klen, void *v,
		   size_t vlen);
int kvsal_set_binary(char *k, char *buf, size_t size);
int kvsal_get_binary(char *k, char *buf, size_t *size);
int kvsal_set_stat(char *k, struct stat *buf);
int kvsal2_set_stat(void *ctx, char *k, size_t klen, struct stat *buf);
int kvsal_get_stat(char *k, struct stat *buf);
int kvsal2_get_stat(void *ctx, char *k, size_t klen, struct stat *buf);
int kvsal_get_list_size(char *pattern);
int kvsal2_get_list_size(void *ctx, char *pattern, size_t size);
int kvsal_del(char *k);
int kvsal2_del(void *ctx, char *k, size_t klen);
int kvsal2_del_bin(void *ctx, const void *key, size_t klen);
int kvsal_incr_counter(char *k, unsigned long long *v);
int kvsal2_incr_counter(void *ctx, char *k, unsigned long long *v);

int kvsal_get_list_pattern(char *pattern, int start, int *end,
			   kvsal_item_t *items);
int kvsal_get_list(kvsal_list_t *list, int start, int *end, kvsal_item_t *items);
int kvsal_fetch_list(char *pattern, kvsal_list_t *list);
int kvsal2_fetch_list(void *ctx, char *pattern, kvsal_list_t *list);
int kvsal_dispose_list(kvsal_list_t *list);
int kvsal_init_list(kvsal_list_t *list);
int kvsal_create_fs_ctx(unsigned long fs_id, void **fs_ctx);


/** Check if the given index (`ctx`) has at least one key with
 * the prefix `kprefix` of len `klen`.
 * @param[in, out] ctx - Index handle.
 * @param[in] kprefix - Prefix of a key.
 * @param[in] klen - Length of the prefix.
 * @param[out] result - Presense of a key with such a prefix.
 * @return 0 if successfull, otherwise -errno.
 */
int kvsal_key_prefix_exists(void *ctx, const void *key, size_t klen,
			    bool *result);

#endif
