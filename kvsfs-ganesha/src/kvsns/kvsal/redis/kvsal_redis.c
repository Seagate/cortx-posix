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

/* kvsal_redis.c
 * KVS Abstraction Layer: interface for REDIS
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <time.h>
#include <ini_config.h>
#include <kvsns/kvsal.h>
#include <assert.h>
#include "kvsns/log.h"

/* The REDIS context exists in the TLS, for MT-Safety */
__thread redisContext *rediscontext = NULL;

static struct collection_item *conf = NULL;

int kvsal_init(struct collection_item *cfg_items)
{
	redisReply *reply;
	char *hostname = NULL;
	char hostname_default[] = "127.0.0.1";
	struct timeval timeout = { 1, 500000 }; /* 1.5 seconds */
	int port = 6379; /* REDIS default */
	struct collection_item *item = NULL;

	if (cfg_items == NULL)
		return -EINVAL;

	if (conf == NULL)
		conf = cfg_items;

	/* Get config from ini file */
	item = NULL;
	RC_WRAP(get_config_item, "kvsal_redis", "server", cfg_items, &item);
	if (item == NULL)
		hostname = hostname_default;
	else
		hostname = get_string_config_value(item, NULL);

	item = NULL;
	RC_WRAP(get_config_item, "kvsal_redis", "port", cfg_items, &item);
	if (item != NULL)
		port = (int)get_int_config_value(item, 0, 0, NULL);

	/* Start REDIS */
	rediscontext = redisConnectWithTimeout(hostname, port, timeout);
	if (rediscontext == NULL || rediscontext->err) {
		if (rediscontext) {
			fprintf(stderr,
				"Connection error: %s\n", rediscontext->errstr);
			redisFree(rediscontext);
		} else {
			fprintf(stderr,
				"Connection error: can't get redis context\n");
		}
		exit(1);
	}

	/* PING server */
	reply = redisCommand(rediscontext, "PING");
	if (!reply)
		return -1;

	log_debug("kvsal init done");

	freeReplyObject(reply);

	return 0;
}

static int kvsal_reinit()
{
	return kvsal_init(conf);
}

int kvsal_fini(void)
{
	return 0;
}

#ifdef KVSNS_ENABLE_REDIS_TRANSACTIONS

int kvsal_begin_transaction(void)
{
	redisReply *reply;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;
	
	log_debug("kvsal begin transaction");

	reply = redisCommand(rediscontext, "MULTI");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STATUS) {
		freeReplyObject(reply);
		return -1;
	}

	if (strncmp(reply->str, "OK", reply->len)) {
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);

	return 0;
}


int kvsal_end_transaction(void)
{
	redisReply *reply;
	int i;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	log_debug("kvsal end transaction");

	reply = redisCommand(rediscontext, "EXEC");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_ARRAY) {
		freeReplyObject(reply);
		return -1;
	}

	for (i = 0; i < reply->elements ; i++)
		if (strncmp(reply->element[i]->str, "OK",
			    reply->element[i]->len)) {
			freeReplyObject(reply);
			return -1;
		}

	freeReplyObject(reply);

	return 0;
}

int kvsal_discard_transaction(void)
{
	redisReply *reply;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	log_debug("kvsal discard transaction");

	reply = redisCommand(rediscontext, "DISCARD");
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STATUS) {
		freeReplyObject(reply);
		return -1;
	}

	if (strncmp(reply->str, "OK", reply->len)) {
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);

	return 0;
}

#else

int kvsal_begin_transaction(void)
{
	log_debug("kvsal begin transaction");
	
	return 0;
}


int kvsal_end_transaction(void)
{
	log_debug("kvsal end transaction");

	return 0;
}


int kvsal_discard_transaction(void)
{
	log_debug("kvsal discard transaction");
	return 0;
}

#endif

int kvsal2_exists(void *ctx, char *k, size_t klen)
{
	redisReply *reply;

	assert(k);

	assert(rediscontext);

	log_debug("EXISTS %s", k);

	if (kvsal_reinit() != 0)
		return -EIO;

	/* Set a key */
	reply = redisCommand(rediscontext, "EXISTS %b", k, klen);
	if (!reply)
		return -EIO;

	if (reply->type != REDIS_REPLY_INTEGER)
		return -EIO;

	if (reply->integer == 0)
		return -ENOENT;
	log_debug("EXISTS %s, reply: %llu", k, reply->integer);

	freeReplyObject(reply);

	return 0;
}

int kvsal_set_char(char *k, char *v)
{
	redisReply *reply;
	size_t klen;
	size_t vlen;

	klen = strnlen(k, KLEN);
	vlen = strnlen(v, VLEN);

	assert(k && v);

	assert(rediscontext);
	
	log_debug("SET key = %s, value = %s", k, v);
	
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", k, klen, v, vlen);
	if (!reply)
		return -EIO;
	log_debug("SET key = %s, value = %s reply: %s", k, v, reply->str);

	freeReplyObject(reply);

	return 0;
}

int kvsal2_set_char(void *ctx, char *k, size_t klen, char *v, size_t vlen)
{
	redisReply *reply;
	
	assert(k && v);
	
	assert(rediscontext);
	log_debug("SET key = %s, value = %s ", k, v);
	
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", k, klen, v, vlen);
	if (!reply)
		return -EIO;

	log_debug("SET key = %s, value = %s reply: %s", k, v, reply->str);

	freeReplyObject(reply);

	return 0;
}

int kvsal_get_char(char *k, char *v)
{
	redisReply *reply;
	size_t klen;
	
	klen = strnlen(k, KLEN);

	assert(k && v);

	assert(rediscontext);
		
	log_debug("GET key = %s", k);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Try a GET and two INCR */
	reply = NULL;
	reply = redisCommand(rediscontext, "GET %b", k, klen);
	if (!reply)
		return -EIO;

	if (reply->len == 0)
		return -ENOENT;

	strcpy(v, reply->str);
	
	log_debug("GET key = %s, reply: %s", k,  reply->str);

	freeReplyObject(reply);

	return 0;
}

int kvsal_set_stat(char *k, struct stat *buf)
{
	redisReply *reply;
	
	size_t size = sizeof(struct stat);

        size_t klen;

        klen = strnlen(k, KLEN);
	log_debug("SET key = %s, value = %s ", k, (char *) buf);

	if (!k || !buf)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", k, klen, buf, size);
	if (!reply)
		return -1;

	log_debug("SET key = %s, value = %s reply: %s", k, (char *) buf, reply->str);

	freeReplyObject(reply);

	return 0;
}

int kvsal_get_stat(char *k, struct stat *buf)
{
	redisReply *reply;
	size_t klen;

        klen = strnlen(k, KLEN);
	log_debug("GET key = %s", k);

	if (!k || !buf)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "GET %b", k, klen);
	if (!reply)
		return -1;

	if (reply->type != REDIS_REPLY_STRING)
		return -1;

	if (reply->len != sizeof(struct stat))
		return -1;

	memcpy((char *)buf, reply->str, reply->len);

	log_debug("GET key = %s, reply: %s", k,  reply->str);

	freeReplyObject(reply);

	return 0;
}

int kvsal_set_binary(char *k, char *buf, size_t size)
{
	redisReply *reply;
	size_t klen;

	klen = strnlen(k, KLEN);
	
	assert(k && buf);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", k, klen, buf, size);
	if (!reply)
		return -EIO;

	log_debug("SET key = %s, value = %s reply: %s", k, buf, reply->str);

	freeReplyObject(reply);
	
	return 0;
}

int kvsal_get_binary(char *k, char *buf, size_t *size)
{
	redisReply *reply;
	size_t klen;

	klen = strnlen(k, KLEN);

	assert(k && buf && size);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	reply = redisCommand(rediscontext, "GET %b", k, klen);
	if (!reply)
		return -EIO;

	if (reply->type != REDIS_REPLY_STRING)
		return -1;

	if (reply->len > *size)
		return -1;

	memcpy((char *)buf, reply->str, reply->len);
	*size = reply->len;
	
	log_debug("GET key = %s reply: %s", k,  reply->str);

	freeReplyObject(reply);

	return 0;
}

int kvsal3_set_bin(void *ctx, void *k, const size_t klen, void *v,
		   const size_t vlen)
{
	redisReply *reply;

	assert(k && v);
	
	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", (char *) k, klen, (char *) v, vlen);
	if (!reply)
		return -EIO;
	
	log_debug("SET key = %s, value = %s reply: %s", (char *) k, (char *) v, reply->str);
	
	freeReplyObject(reply);
	return 0;
}


int kvsal2_set_bin(void *ctx, const void *k, size_t klen, const void *v,
		   size_t vlen)
{
	redisReply *reply;

	assert(k && v);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", (char *) k, klen, (char *) v, vlen);
	if (!reply)
		return -EIO;
	
	log_debug("SET key = %s, value = %s reply: %s", (char *) k, (char *) v, reply->str);

	freeReplyObject(reply);
	return 0;
}

int kvsal2_get_bin(void *ctx, const void *k, size_t klen, void *v, size_t vlen)
{
	redisReply *reply;

	assert(k && v && vlen);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	reply = redisCommand(rediscontext, "GET %b", (char *) k, klen);
	if (!reply)
		return -EIO;

	if (reply->type == REDIS_REPLY_NIL)
		return -ENOENT;

	if (reply->type != REDIS_REPLY_STRING)
		return -EIO;

	if (reply->len > vlen)
		return -EIO;

	memcpy((char *)v, reply->str, reply->len);
	vlen = reply->len;

	log_debug("GET key = %s reply: %s", (char *) k, reply->str);

	freeReplyObject(reply);


	return 0;
}

int kvsal3_get_bin(void *ctx, void *k, const size_t klen, void **v,
		  size_t *vlen)
{
	redisReply *reply;
	char *buf;

	assert(k && v && vlen);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	reply = redisCommand(rediscontext, "GET %b", (char *) k, klen);
	if (!reply)
		return -EIO;
	
	if (reply->type == REDIS_REPLY_NIL)
		return -ENOENT;

	if (reply->type != REDIS_REPLY_STRING)
		return -EIO;
	
	/* Verify the value len if the caller know it  */
	if (*vlen != 0) 
  		assert(reply->len <= *vlen);

	buf = malloc(sizeof(char) * reply->len);	
	if (buf == NULL)
		return -EIO;

	memcpy(buf, reply->str, reply->len);
	*v = buf;

	*vlen = reply->len;
	
	log_debug("GET key = %s, reply: %s",(char *)  k,  reply->str);

	freeReplyObject(reply);
	return 0;
}

int kvsal2_incr_counter(void *ctx, char *k, unsigned long long *v)
{
	redisReply *reply;
	size_t klen;

	klen = strnlen(k, KLEN);
	
	assert(k && v);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	reply = redisCommand(rediscontext, "INCR %b", k, klen);
	if (!reply)
		return -EIO;

	*v = (unsigned long long)reply->integer;
	log_debug("inode counter=%llu", *v);

	return 0;
}

int kvsal_del(char *k)
{
	redisReply *reply;
	size_t klen;

	klen = strnlen(k, KLEN)+1;

	assert(k);
	
	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Try a GET and two INCR */
	reply = redisCommand(rediscontext, "DEL %b", k, klen);
	if (!reply)
		return -EIO;
	
	log_debug("DEL key = %s, reply: %s", k, reply->str);

	freeReplyObject(reply);
	return 0;
}

int kvsal2_del(void *ctx, char *k, size_t klen)
{
	redisReply *reply;

	assert(k);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Try a GET and two INCR */
	reply = redisCommand(rediscontext, "DEL %b", k, klen);
	if (!reply)
		return -EIO;
	
	log_debug("DEL key = %s, reply: %s", k, reply->str);

	freeReplyObject(reply);
	return 0;
}

int kvsal2_del_bin(void *ctx, const void *key, size_t klen)
{
	redisReply *reply;

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -EIO;

	/* Try a GET and two INCR */
	reply = redisCommand(rediscontext, "DEL %b",(char *) key, klen);
	if (!reply)
		return -EIO;
	
	log_debug("DEL key = %s,  reply: %s", (char *) key, reply->str);

	freeReplyObject(reply);
	return 0;
}

int kvsal_get_list_pattern(char *pattern, int start, int *size,
			   kvsal_item_t *items)
{
	redisReply *reply;
	int i;

	if (!pattern || !size || !items)
		return -EINVAL;

	if (!rediscontext)
		if (kvsal_reinit() != 0)
			return -1;

	reply = redisCommand(rediscontext, "KEYS %s", pattern);
	if (!reply)
		return -1;
	if (reply->type != REDIS_REPLY_ARRAY)
		return -1;

	if (reply->elements < (start + *size))
		*size = reply->elements - start;

	for (i = start; i < start + *size ; i++) {
		items[i-start].offset = i;
		strncpy(items[i-start].str, reply->element[i]->str, KLEN);
	}
	return 0;
}

int kvsal2_get_list_size(void *ctx, char *pattern, size_t plen)
{
	redisReply *reply;
	int rc;

	assert(pattern);

	assert(rediscontext);
	if (kvsal_reinit() != 0)
		return -1;

	reply = redisCommand(rediscontext, "KEYS %s", pattern);
	if (!reply)
		return -1;
	if (reply->type != REDIS_REPLY_ARRAY)
		return -1;
	rc = reply->elements;

	log_debug("KEYS pattern = %s reply: %s", pattern, reply->str);

	freeReplyObject(reply);
	return rc;
}

int kvsal_init_list(kvsal_list_t *list)
{
	if (!list)
		return -EINVAL;

	list->size = 0;
	list->content = NULL;

	return 0;
}

int kvsal_fetch_list(char *pattern, kvsal_list_t *list)
{
	if (!pattern || !list)
		return -EINVAL;

	/* REDIS manages KVS in RAM. Nothing to do */
	strncpy(list->pattern , pattern, KLEN);

	return 0;
}

int kvsal_dispose_list(kvsal_list_t *list)
{
	if (!list)
		return -EINVAL;

	/* REDIS manages KVS in RAM. Nothing to do */
	return 0;
}

int kvsal_get_list(kvsal_list_t *list, int start, int *end,
		    kvsal_item_t *items)
{
	if (!list)
		return -EINVAL;
	
	return kvsal_get_list_pattern(list->pattern, 
				      start, 
				      end,
				      items);
}

size_t kvsal_iter_get_key(struct kvsal_iter *iter, void **buf)
{
	return 0;
}

bool kvsal_prefix_iter_find(struct kvsal_prefix_iter *iter)
{
	return 0;
}

int kvsal_alloc(void **ptr, uint64_t size)
{
	int rc = 0;

       *ptr = malloc(size);
       if (*ptr == NULL)
               rc = -ENOMEM;
	
       log_debug("alloc rc:%d", rc);

       return rc;
}

void kvsal_free(void *ptr)
{
	free(ptr);
	log_debug("free done");
}

size_t kvsal_iter_get_value(struct kvsal_iter *iter, void **buf)
{
	return 0;
}

bool kvsal_prefix_iter_next(struct kvsal_prefix_iter *iter)
{
	return true;
}

void kvsal_prefix_iter_fini(struct kvsal_prefix_iter *iter)
{
	
}

int kvsal_create_fs_ctx(unsigned long fs_id, void **fs_ctx)
{
	return 0;
}
