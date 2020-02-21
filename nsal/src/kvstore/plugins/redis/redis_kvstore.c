/*
 * Filename: redis_kvstore.c
 * Description:      Implementation of Redis KVStore.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains APIs which implement NSAL's KVStore framework,
 on top of Redis.
*/

#include "redis/redis_kvstore.h"
#include <hiredis/hiredis.h>
#include <ini_config.h>
#include <debug.h>
#include <common/log.h>
#include <string.h>

/* The REDIS context exists in the TLS, for MT-Safety */
__thread redisContext *rediscontext = NULL;

static struct collection_item *conf = NULL;

struct redis_iter_priv
{
	struct redisReply *prev_scan;
	struct redisReply *prev_get;
};

int redis_init(struct collection_item *cfg_items)
{
	redisReply *reply;
	char *hostname = NULL;
	char hostname_default[] = "127.0.0.1";
	struct timeval timeout = { 1, 500000 }; /* 1.5 seconds */
	int port = 6379; /* REDIS default */
	struct collection_item *item = NULL;

	if (cfg_items == NULL) {
		return -EINVAL;
	}

	if (conf == NULL) {
		conf = cfg_items;
	}

	/* Get config from ini file */
	item = NULL;
	RC_WRAP(get_config_item, "kvsal_redis", "server", cfg_items, &item);
	if (item == NULL) {
		hostname = hostname_default;
	} else {
		hostname = get_string_config_value(item, NULL);
	}

	item = NULL;

	RC_WRAP(get_config_item, "kvsal_redis", "port", cfg_items, &item);
	if (item != NULL) {
		port = (int)get_int_config_value(item, 0, 0, NULL);
	}

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
	if (!reply) {
		return -EIO;
	}
	log_info("redis kvstore init done");
	freeReplyObject(reply);

	return 0;
}

static int redis_reinit()
{
	return redis_init(conf);
}

int redis_fini(void)
{
	return 0;
}

int redis_alloc(void **ptr, uint64_t size)
{
	int rc = 0;

	*ptr = malloc(size);
	if (*ptr == NULL) {
		rc = -ENOMEM;
	}
	log_debug("alloc rc:%d", rc);

	return rc;
}

void redis_free(void *ptr)
{
	free(ptr);
	log_debug("free done");
}

#ifdef KVSNS_ENABLE_REDIS_TRANSACTIONS

int redis_begin_transaction(struct kvs_idx *index)
{
	redisReply *reply;

	if (!rediscontext) {
		if (redis_reinit() != 0) {
			return -EIO;
		}
	}

	log_debug("redis begin transaction");

	reply = redisCommand(rediscontext, "MULTI");
	if (!reply) {
		return -EIO;
	}

	if (reply->type != REDIS_REPLY_STATUS) {
		freeReplyObject(reply);
		return -EIO;
	}

	if (strncmp(reply->str, "OK", reply->len)) {
		freeReplyObject(reply);
		return -EIO;
	}

	freeReplyObject(reply);

	return 0;
}


int redis_end_transaction(struct kvs_idx *index)
{
	redisReply *reply;
	int i;

	if (!rediscontext)
		if (redis_reinit() != 0)
			return -EIO;

	log_debug("redis end transaction");

	reply = redisCommand(rediscontext, "EXEC");
	if (!reply)
		return -EIO;

	if (reply->type != REDIS_REPLY_ARRAY) {
		freeReplyObject(reply);
		return -EIO;
	}

	for (i = 0; i < reply->elements ; i++)
		if (strncmp(reply->element[i]->str, "OK",
			    reply->element[i]->len)) {
			freeReplyObject(reply);
			return -EIO;
		}

	freeReplyObject(reply);

	return 0;
}

int redis_discard_transaction(struct kvs_idx *index)
{
	redisReply *reply;

	if (!rediscontext)
		if (redis_reinit() != 0)
			return -EIO;

	log_debug("redis discard transaction");

	reply = redisCommand(rediscontext, "DISCARD");
	if (!reply) {
		return -EIO;
	}

	if (reply->type != REDIS_REPLY_STATUS) {
		freeReplyObject(reply);
		return -EIO;
	}

	if (strncmp(reply->str, "OK", reply->len)) {
		freeReplyObject(reply);
		return -EIO;
	}

	freeReplyObject(reply);

	return 0;
}

#else

int redis_begin_transaction(struct kvs_idx *index)
{
	return 0;
}

int redis_end_transaction(struct kvs_idx *index)
{
	return 0;
}

int redis_discard_transaction(struct kvs_idx *index)
{
	return 0;
}

#endif

int redis_index_create(const kvs_fid_t *fid, struct kvs_idx *index)
{
	index->index_priv = rediscontext;
	return 0;
}

int redis_index_delete(const kvs_fid_t *fid)
{
	return 0;
}

int redis_index_open(const kvs_fid_t *fid, struct kvs_idx *index)
{
       if (rediscontext == NULL) {
       return 1;
       }

       index->index_priv = rediscontext;
       return 0;
}

int redis_index_close(struct kvs_idx *index)
{
       return 0;
}

int redis_set_bin(struct kvs_idx *index, void *k, const size_t klen,
                  void *v, const size_t vlen)
{
	redisReply *reply;
	int rc = 0;
	dassert(k && v);

	if (!rediscontext) {
		if (redis_reinit() != 0) {
			rc = -EIO;
			goto out;
		}
	}

	/* Set a key */
	reply = redisCommand(rediscontext, "SET %b %b", (char *) k, klen,
	     (char *) v, vlen);
	if (!reply) {
		rc = -EIO;
		goto out;
	}

	log_debug("SET key = %s, value = %s reply: %s", (char *) k, (char *) v,
	  reply->str);

	freeReplyObject(reply);
out:
	return rc;
}

int redis_get_bin(struct kvs_idx *index, void *k, const size_t klen,
                  void **v, size_t *vlen)
{
	redisReply *reply;
	char *buf;

	dassert(k && v && vlen);

	if (!rediscontext) {
        if (redis_reinit() != 0) {
                return -EIO;
		}
	}

	/* Get value for the given key */
	reply = redisCommand(rediscontext, "GET %b", (char *) k, klen);
	if (!reply) {
        return -EIO;
	}

	if (reply->type == REDIS_REPLY_NIL) {
        return -ENOENT;
	}
	if (reply->type != REDIS_REPLY_STRING) {
        return -EIO;
	}

	/* Verify the value len if the caller know it  */
	if (*vlen != 0) {
        dassert(reply->len <= *vlen);
	}

	buf = malloc(sizeof(char) * reply->len);
	if (buf == NULL) {
        return -EIO;
	}

	memcpy(buf, reply->str, reply->len);
	*v = buf;

	*vlen = reply->len;

	log_debug("GET key = %s, reply: %s",(char *)  k,  reply->str);

	freeReplyObject(reply);
	return 0;
}

int redis_del_bin(struct kvs_idx *index, const void *key, size_t klen)
{
	redisReply *reply;

	if (!rediscontext) {
		if (redis_reinit() != 0) {
			return -EIO;
		}
	}

	reply = redisCommand(rediscontext, "DEL %b",(char *) key, klen);
	if (!reply) {
		return -EIO;
	}

	log_debug("DEL key = %s,  reply: %s", (char *) key, reply->str);

	freeReplyObject(reply);
	return 0;
}

int redis4_set_bin(void *k, const size_t klen, void *v, const size_t vlen)
{
	return redis_set_bin(NULL, k, klen, v, vlen);
}

int redis4_get_bin(void *k, const size_t klen, void **v, size_t *vlen)
{
	return redis_get_bin(NULL, k, klen, v, vlen);
}

/******************************************************************************/
/* Key Iterator */

void redis_iter_get_kv(struct kvs_itr *iter, void **key, size_t *klen,
                       void **val, size_t *vlen)
{
	struct redis_iter_priv *priv = (void *) iter->priv;

	*key = priv->prev_scan->element[1]->element[0]->str;
	*klen = priv->prev_scan->element[1]->element[0]->len;

	*val = priv->prev_get->str;
	*vlen = priv->prev_get->len;

	log_debug("length of inode %zu", priv->prev_get->len);
}

int redis_prefix_iter_has_prefix(struct kvs_itr *iter)
{
	void *key, *value;
	size_t key_len, val_len;

	redis_iter_get_kv(iter, &key, &key_len, &value, &val_len);
	dassert(key_len >= iter->prefix.len);

	return memcmp(iter->prefix.buf, key, iter->prefix.len);
}

/** Find the first record following by the prefix and set iter to it.
 * @param iter Iterator object to initialized with the starting record.
 * @param prefix Key prefix to be found.
 * @param prefix_len Length of the prefix.
 * @return True if found, otherwise False. @see kvs_itr::inner_rc for return
 * code.
 */
int redis_prefix_iter_find(struct kvs_itr *iter)
{
	redisReply *reply;
	struct redis_iter_priv *priv = (void *) iter->priv;
	int rc = 0;
	char cur[8];
	strcpy(cur, "0");
	bool key_present = false;

	do {
		reply = redisCommand(rediscontext, "SCAN %s MATCH %b* COUNT 1", cur,
		                     iter->prefix.buf, iter->prefix.len);
		if (!reply || rediscontext->err ) {
			rc = -EIO;
			goto out;
		}

		strcpy(cur, reply->element[0]->str);

		if (reply->element[1]->elements != 0) {

			key_present = true;
			break;
		}
	}while (strcmp(cur, "0"));

	if (!key_present) {
		rc = -ENOENT;
		goto out;
	}
	priv->prev_scan = reply;
	reply = redisCommand(rediscontext, "GET %b",
	                     priv->prev_scan->element[1]->element[0]->str,
						 priv->prev_scan->element[1]->element[0]->len);

	if (!reply || rediscontext->err ) {
		rc = -EIO;
		goto out;
	}

	priv->prev_get = reply;

	/* release objects back to priv */
	reply = NULL;

	rc = redis_prefix_iter_has_prefix(iter);
	if (rc) {
		goto out;
	}

	rc = (key_present == true && rc == 0) ? 0 : rc;
out:
	if (reply) {
		freeReplyObject(reply);
	}

	return rc;
}

/* Find the next record and set iter to it. */
int redis_prefix_iter_next(struct kvs_itr *iter)
{
	redisReply *reply;
	struct redis_iter_priv *priv = (void *) iter->priv;
	bool can_get_next = false;
	char cur[8];
	int rc = 0;

	strcpy(cur, priv->prev_scan->element[0]->str);
	if (strcmp(cur, "0") == 0) {
		rc = -ENOENT;
		goto out;
	}

	do{
		reply = redisCommand(rediscontext, "SCAN %s MATCH %b* COUNT 1", cur,
		                     iter->prefix.buf, iter->prefix.len);
		if (!reply || rediscontext->err ) {
			rc = -EIO;
			goto out;
		}

		strcpy(cur, reply->element[0]->str);
		if (reply->element[1]->elements != 0) {
			can_get_next = true;
			break;
		}

	}while (strcmp(cur, "0"));

	if (!can_get_next) {
		rc = -ENOENT;
		goto out;
	}

	priv->prev_scan = reply;

	reply = redisCommand(rediscontext, "GET %b",
	                     priv->prev_scan->element[1]->element[0]->str,
						 priv->prev_scan->element[1]->element[0]->len);

	if (!reply || rediscontext->err ) {
		rc = -EIO;
		goto out;
	}

	priv->prev_get = reply;

	/* release objects back to priv */
	reply = NULL;

	rc = redis_prefix_iter_has_prefix(iter);
	rc = (can_get_next == true && rc == 0) ? 0 : rc;
out:
	if (reply) {
		freeReplyObject(reply);
	}
	return rc;
}

/** Cleanup key iterator object */
void redis_prefix_iter_fini(struct kvs_itr *iter)
{
	struct redis_iter_priv *priv = (void *) iter->priv;

	freeReplyObject(priv->prev_scan);
	freeReplyObject(priv->prev_get);
}

struct kvstore_ops redis_kvs_ops = {
	.init = redis_init,
	.fini = redis_fini,
	.alloc = redis_alloc,
	.free = redis_free,
	.index_create = redis_index_create,
	.index_delete = redis_index_delete,
	.index_open = redis_index_open,
	.index_close = redis_index_close,
	.begin_transaction = redis_begin_transaction,
	.end_transaction = redis_end_transaction,
	.discard_transaction = redis_discard_transaction,
	.get_bin = redis_get_bin,
	.get4_bin = redis4_get_bin,
	.set_bin = redis_set_bin,
	.set4_bin = redis4_set_bin,
	.del_bin = redis_del_bin,
	.kv_find = redis_prefix_iter_find,
	.kv_next = redis_prefix_iter_next,
	.kv_fini = redis_prefix_iter_fini,
	.kv_get = redis_iter_get_kv
};

