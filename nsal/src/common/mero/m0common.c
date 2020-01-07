/* -*- C -*- */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syscall.h> /* for gettid */
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <dirent.h>

#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"
#include "lib/thread.h"
#include "m0common.h"
#include <mero/helpers/helpers.h>
//#include "kvsns/log.h"

struct clovis_io_ctx {
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
};

enum {
	 KVS_FID_STR_LEN = 128
};

/* To be passed as argument */
struct m0_clovis_realm     clovis_uber_realm;

static pthread_once_t clovis_init_once = PTHREAD_ONCE_INIT;
bool clovis_init_done = false;
__thread struct m0_thread m0thread;
__thread bool my_init_done = false;

static pthread_t m0init_thread;
static struct collection_item *conf = NULL;

static struct m0_fid ifid;
static struct m0_clovis_idx idx;

static char *clovis_local_addr;
static char *clovis_ha_addr;
static char *clovis_prof;
static char *clovis_proc_fid;
static char *clovis_index_dir = "/tmp/";
static char *ifid_str;

/* Clovis Instance */
static struct m0_clovis	  *clovis_instance = NULL;

/* Clovis container */
static struct m0_clovis_container clovis_container;

/* Clovis Configuration */
static struct m0_clovis_config	clovis_conf;
static struct m0_idx_dix_config	dix_conf;

struct m0_clovis_realm     clovis_uber_realm;

static struct m0_ufid_generator kvsns_ufid_generator;

#define WRAP_CONFIG(__name, __cfg, __item) ({\
	int __rc = get_config_item("mero", __name, __cfg, &__item);\
	if (__rc != 0)\
		return -__rc;\
	if (__item == NULL)\
		return -EINVAL; })

/* Static functions */

static int get_clovis_conf(struct collection_item *cfg)
{
	struct collection_item *item;

	if (cfg == NULL)
		return -EINVAL;

	item = NULL;
	WRAP_CONFIG("local_addr", cfg, item);
	clovis_local_addr = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("ha_addr", cfg, item);
	clovis_ha_addr = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("profile", cfg, item);
	clovis_prof = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("proc_fid", cfg, item);
	clovis_proc_fid = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("index_dir", cfg, item);
	clovis_index_dir = get_string_config_value(item, NULL);

	item = NULL;
	WRAP_CONFIG("kvs_fid", cfg, item);
	ifid_str = get_string_config_value(item, NULL);

	return 0;
}

static void print_config(void)
{
	printf("local_addr = %s\n", clovis_local_addr);
	printf("ha_addr    = %s\n", clovis_ha_addr);
	printf("profile    = %s\n", clovis_prof);
	printf("proc_fid   = %s\n", clovis_proc_fid);
	printf("index_dir  = %s\n", clovis_index_dir);
	printf("kvs_fid    = %s\n", ifid_str);
	printf("---------------------------\n");
}

int m0kvs_reinit(void)
{
	return m0init(conf);
}

static int init_clovis(void)
{
	int rc;
	char  tmpfid[MAXNAMLEN];

	assert(clovis_local_addr && clovis_ha_addr && clovis_prof &&
	       clovis_proc_fid);

	/* Initialize Clovis configuration */
	clovis_conf.cc_is_oostore	= true;
	clovis_conf.cc_is_read_verify	= false;
	clovis_conf.cc_local_addr	= clovis_local_addr;
	clovis_conf.cc_ha_addr		= clovis_ha_addr;
	clovis_conf.cc_profile		= clovis_prof;
	clovis_conf.cc_process_fid       = clovis_proc_fid;
	clovis_conf.cc_tm_recv_queue_min_len    = M0_NET_TM_RECV_QUEUE_DEF_LEN;
	clovis_conf.cc_max_rpc_msg_size	 = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
	clovis_conf.cc_layout_id	= 0;

	/* Index service parameters */
	clovis_conf.cc_idx_service_id	= M0_CLOVIS_IDX_DIX;
	dix_conf.kc_create_meta		= false;
	clovis_conf.cc_idx_service_conf	= &dix_conf;

	/* Create Clovis instance */
	rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);
	if (rc != 0) {
		fprintf(stderr, "Failed to initilise Clovis\n");
		goto err_exit;
	}

	/* Container is where Entities (object) resides.
	 * Currently, this feature is not implemented in Clovis.
	 * We have only single realm: UBER REALM. In future with multiple realms
	 * multiple applications can run in different containers. */
	m0_clovis_container_init(&clovis_container,
				 NULL, &M0_CLOVIS_UBER_REALM,
				 clovis_instance);

	rc = clovis_container.co_realm.re_entity.en_sm.sm_rc;
	if (rc != 0) {
		fprintf(stderr, "Failed to open uber realm\n");
		goto err_exit;
	}

	clovis_uber_realm = clovis_container.co_realm;

	/* Get fid from config parameter */
	memset(&ifid, 0, sizeof(struct m0_fid));
	rc = m0_fid_sscanf(ifid_str, &ifid);
	if (rc != 0) {
		fprintf(stderr, "Failed to read ifid value from conf\n");
		goto err_exit;
	}

	rc = m0_fid_print(tmpfid, MAXNAMLEN, &ifid);
	if (rc < 0) {
		fprintf(stderr, "Failed to read ifid value from conf\n");
		goto err_exit;
	}

	m0_clovis_idx_init(&idx, &clovis_container.co_realm,
			   (struct m0_uint128 *)&ifid);

	rc = m0_ufid_init(clovis_instance, &kvsns_ufid_generator);
	if (rc != 0) {
		fprintf(stderr, "Failed to initialise fid generator: %d\n", rc);
		goto err_exit;
	}

	return 0;

err_exit:
	return rc;
}

static void m0kvs_do_init(void)
{
	int rc;

	rc = get_clovis_conf(conf);

	if (rc != 0) {
		fprintf(stderr, "Invalid config file\n");
		exit(1);
	}

	print_config();

	rc = init_clovis();
	assert(rc == 0);

	clovis_init_done = true;
	m0init_thread = pthread_self();
}


static int m0_op_kvs(enum m0_clovis_idx_opcode opcode,
		     struct m0_bufvec *key,
		     struct m0_bufvec *val)
{
	struct m0_clovis_op	 *op = NULL;
	int rcs[1];
	int rc;

	if (!my_init_done)
		m0kvs_reinit();

	rc = m0_clovis_idx_op(&idx, opcode, key, val,
			      rcs, M0_OIF_OVERWRITE, &op);
	if (rc)
		return rc;

	m0_clovis_op_launch(&op, 1);
	rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	if (rc)
		goto out;

	/* Check rcs array even if op is succesful */
	rc = rcs[0];

out:
	m0_clovis_op_fini(op);
	/* it seems like 0_free(&op) is not needed */
	return rc;
}


/* @todo: Creates an clovis index for a fs_id. Currently
 * this only returns a global idx */
int m0idx_get_global(struct m0_clovis_idx **index)
{
	*index = &idx;
	return 0;
}

int m0idx_create(const struct m0_uint128 *fid, struct m0_clovis_idx **index)
{
        int                     rc;
	struct m0_clovis_op    *op;
	struct m0_clovis_idx   *idx = NULL;

	*index = NULL;

	idx = m0kvs_alloc(sizeof(struct m0_clovis_idx));
	if (idx == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	/* Set an index creation operation. */
	m0_clovis_idx_init(idx,
			&clovis_container.co_realm, (struct m0_uint128 *)fid);

        rc = m0_clovis_entity_create(NULL, &(idx->in_entity), &op);
	if (rc == 0) {
 		/* Launch and wait for op to complete */
        	m0_clovis_op_launch(&op, 1);
        	rc = m0_clovis_op_wait(op,
        		M0_BITS(M0_CLOVIS_OS_FAILED,
                	M0_CLOVIS_OS_STABLE),
                	M0_TIME_NEVER);
		if (rc == 0) {
			rc = op->op_rc;
		}
	}

        /* fini and release */
        m0_clovis_op_fini(op);
        m0_clovis_op_free(op);

	if (rc) {
		m0kvs_free(idx);
	} else {
		*index = idx;
	}

out:
        return rc;
}

int m0idx_delete(const struct m0_uint128 *fid)
{
        int                     rc;
        struct m0_clovis_op     *op;
        struct m0_clovis_idx    idx;

        memset(&idx, 0, sizeof(struct m0_clovis_idx));

        /* Set an index creation operation. */
        m0_clovis_idx_init(&idx,
                &clovis_container.co_realm, (struct m0_uint128 *)fid);

	rc = m0_clovis_entity_open(&idx.in_entity, &op);
	if (rc != 0) {
		goto out;
	}

        rc = m0_clovis_entity_delete(&(idx.in_entity), &op);
	if (rc == 0) {
        	/* Launch and wait for op to complete */
        	m0_clovis_op_launch(&op, 1);
        	rc = m0_clovis_op_wait(op,
        		M0_BITS(M0_CLOVIS_OS_FAILED,
                	M0_CLOVIS_OS_STABLE),
                	M0_TIME_NEVER);
		if (rc == 0) {
        		rc = op->op_rc;
		}
	}

        /* fini and release */
        m0_clovis_op_fini(op);
        m0_clovis_op_free(op);

out:
        m0_clovis_entity_fini(&(idx.in_entity));
        return rc;
}

int m0idx_open(const struct m0_uint128 *fid, struct m0_clovis_idx **index)
{
	int 			rc = 0;
	struct m0_clovis_idx  	*idx = NULL;
	
	*index = NULL;

	idx = m0kvs_alloc(sizeof(struct m0_clovis_idx));
        if (idx == NULL) {
                rc = -ENOMEM;
		goto out;
        }

        m0_clovis_idx_init(idx, &clovis_container.co_realm,
                           (struct m0_uint128 *)fid);

	*index = idx;

out:
	return rc;
}

void m0idx_close(struct m0_clovis_idx *index)
{
        m0_clovis_idx_fini(index);
	m0kvs_free(index);
}

static int m0_op2_kvs(void *ctx,
		      enum m0_clovis_idx_opcode opcode,
		      struct m0_bufvec *key,
		      struct m0_bufvec *val)
{
	struct m0_clovis_op	 *op = NULL;
	int rcs[1];
	int rc;

	struct m0_clovis_idx     *index = NULL;

	if (!my_init_done)
		m0kvs_reinit();

	index = ctx;

	rc = m0_clovis_idx_op(index, opcode, key, val,
			      rcs, M0_OIF_OVERWRITE, &op);
	if (rc)
		return rc;

	m0_clovis_op_launch(&op, 1);
	rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	if (rc)
		goto out;

	/* Check rcs array even if op is succesful */
	rc = rcs[0];

out:
	m0_clovis_op_fini(op);
	/* it seems like 0_free(&op) is not needed */
	return rc;
}

/* Non-static function starts here */
static pthread_mutex_t m0init_lock = PTHREAD_MUTEX_INITIALIZER;

char * m0_get_gfid()
{
	return strdup(ifid_str);
}

int m0init(struct collection_item *cfg_items)
{
	if (cfg_items == NULL)
		return -EINVAL;

	if (conf == NULL)
		conf = cfg_items;

	(void) pthread_once(&clovis_init_once, m0kvs_do_init);

	pthread_mutex_lock(&m0init_lock);

	if (clovis_init_done && (pthread_self() != m0init_thread)) {
		printf("==========> tid=%d I am not the init thread\n",
		       (int)syscall(SYS_gettid));

		memset(&m0thread, 0, sizeof(struct m0_thread));

		m0_thread_adopt(&m0thread, clovis_instance->m0c_mero);
	} else
		printf("----------> tid=%d I am the init thread\n",
		       (int)syscall(SYS_gettid));

	pthread_mutex_unlock(&m0init_lock);

	my_init_done = true;

	return 0;
}

void m0fini(void)
{
	if (pthread_self() == m0init_thread) {
		/* Finalize Clovis instance */
		m0_clovis_idx_fini(&idx);
		m0_clovis_fini(clovis_instance, true);
	} else
		m0_thread_shun();
}

int m0kvs_get(void *ctx, void *k, size_t klen,
	       void **v, size_t *vlen)
{
	m0_bcount_t k_len = klen;
	struct m0_bufvec key, val;
	int rc;

	// @todo: Assert is called when NFS Ganesha is run.
	// Once issue is debugged uncomment the M0_DASSERT call.
	if (!my_init_done)
		m0kvs_reinit();
	//M0_DASSERT(my_init_done);

	key = M0_BUFVEC_INIT_BUF(&k, &k_len);
	val = M0_BUFVEC_INIT_BUF(v, vlen);

	rc = m0_op2_kvs(ctx, M0_CLOVIS_IC_GET, &key, &val);
	if (rc != 0)
		goto out;

out:
	return rc;
}

int m0kvs4_get(void *k, size_t klen,
	       void **v, size_t *vlen)
{
	m0_bcount_t k_len = klen;
	struct m0_bufvec key, val;
	int rc;

	// @todo: Assert is called when NFS Ganesha is run.
	// Once issue is debugged uncomment the M0_DASSERT call.
	if (!my_init_done)
		m0kvs_reinit();
	//M0_DASSERT(my_init_done);

	key = M0_BUFVEC_INIT_BUF(&k, &k_len);
	val = M0_BUFVEC_INIT_BUF(v, vlen);

	rc = m0_op_kvs(M0_CLOVIS_IC_GET, &key, &val);
	if (rc != 0)
		goto out;

out:
	return rc;
}

int m0kvs4_set(void *k, const size_t klen,
	       void *v, const size_t vlen)
{
	m0_bcount_t k_len = klen;
	m0_bcount_t v_len = vlen;
	struct m0_bufvec key, val;
	int rc;

	M0_DASSERT(my_init_done);

	key = M0_BUFVEC_INIT_BUF(&k, &k_len);
	val = M0_BUFVEC_INIT_BUF(&v, &v_len);

	rc = m0_op_kvs(M0_CLOVIS_IC_PUT, &key, &val);
	return rc;
}

int m0kvs_set(void *ctx, void *k, const size_t klen,
	       void *v, const size_t vlen)
{
	m0_bcount_t k_len = klen;
	m0_bcount_t v_len = vlen;
	struct m0_bufvec key, val;
	int rc;

	M0_DASSERT(my_init_done);

	key = M0_BUFVEC_INIT_BUF(&k, &k_len);
	val = M0_BUFVEC_INIT_BUF(&v, &v_len);

	rc = m0_op2_kvs(ctx, M0_CLOVIS_IC_PUT, &key, &val);
	return rc;
}

int m0kvs_del(void *ctx, void *k, const size_t klen)
{
	struct m0_bufvec key;
	m0_bcount_t k_len = klen;
	int rc;

	M0_DASSERT(my_init_done);

	key = M0_BUFVEC_INIT_BUF(&k, &k_len);

	rc = m0_op2_kvs(ctx, M0_CLOVIS_IC_DEL, &key, NULL);
	return rc;
}


int m0_pattern_kvs(void *ctx, char *k, char *pattern,
		    get_list_cb cb, void *arg_cb)
{
	struct m0_bufvec          keys;
	struct m0_bufvec          vals;
	struct m0_clovis_op       *op = NULL;
	struct m0_clovis_idx      *index = ctx;
	int i = 0;
	int rc;
	int rcs[1];
	bool stop = false;
	char myk[KLEN];
	bool startp = false;
	int size = 0;
	int flags;

	strcpy(myk, k);
	flags = 0; /* Only for 1st iteration */

	do {
		/* Iterate over all records in the index. */
		rc = m0_bufvec_alloc(&keys, 1, KLEN);
		if (rc != 0)
			return rc;

		rc = m0_bufvec_alloc(&vals, 1, VLEN);
		if (rc != 0) {
			m0_bufvec_free(&keys);
			return rc;
		}

		/* FIXME: Memory leak? check m0_bufvec_alloc
		 * documentation. We don't need to allocate
		 * the buffer twice.
		 */
		keys.ov_buf[0] = m0_alloc(strnlen(myk, KLEN)+1);
		keys.ov_vec.v_count[0] = strnlen(myk, KLEN)+1;
		strcpy(keys.ov_buf[0], myk);

		rc = m0_clovis_idx_op(index, M0_CLOVIS_IC_NEXT, &keys, &vals,
				      rcs, flags, &op);
		if (rc != 0) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);
			return rc;
		}
		m0_clovis_op_launch(&op, 1);
		rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
				       M0_TIME_NEVER);
		/* @todo : Why is op null after this call ??? */

		if (rc != 0) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);
			return rc;
		}

		if (rcs[0] == -ENOENT) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);

			/* No more keys to be found */
			if (startp)
				return 0;
			return -ENOENT;
		}
		for (i = 0; i < keys.ov_vec.v_nr; i++) {
			if (keys.ov_buf[i] == NULL) {
				stop = true;
				break;
			}

			/* Small state machine to display things
			 * (they are sorted) */
			if (!fnmatch(pattern, (char *)keys.ov_buf[i], 0)) {

				/* Avoid last one and use it as first
				 *  of next pass */
				if (!stop) {
					if (!cb((char *)keys.ov_buf[i],
						arg_cb))
						break;
				}
				if (startp == false)
					startp = true;
			} else {
				if (startp == true) {
					stop = true;
					break;
				}
			}

			strcpy(myk, (char *)keys.ov_buf[i]);
			flags = M0_OIF_EXCLUDE_START_KEY;
		}

		m0_bufvec_free(&keys);
		m0_bufvec_free(&vals);
	} while (!stop);

	return size;
}

int m0_key_prefix_exists(void *ctx,
			 const void *kprefix, size_t klen,
			 bool *result)
{
	struct m0_bufvec keys;
	struct m0_bufvec vals;
	struct m0_clovis_op *op = NULL;
	struct m0_clovis_idx *index = ctx;
	int rc;
	int rcs[1];

	rc = m0_bufvec_alloc(&keys, 1, klen);
	if (rc != 0) {
		goto out;
	}

	rc = m0_bufvec_empty_alloc(&vals, 1);
	if (rc != 0) {
		goto out_free_keys;
	}

	memset(keys.ov_buf[0], 0, keys.ov_vec.v_count[0]);
	memcpy(keys.ov_buf[0], kprefix, klen);

	rc = m0_clovis_idx_op(index, M0_CLOVIS_IC_NEXT, &keys, &vals,
			      rcs, 0,  &op);

	if (rc != 0) {
		goto out_free_vals;
	}

	if (rcs[0] != 0) {
		goto out_free_vals;
	}

	m0_clovis_op_launch(&op, 1);
	rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	if (rc != 0) {
		goto out_free_op;
	}

	if (rcs[0] == 0) {
		/* The next key cannot be longer than the starting
		 * key by the definition of lexicographical comparison
		 */
		M0_DASSERT(keys.ov_vec.v_count[0] >= klen);
		/* Check if the next key has the same prefix */
		*result = memcmp(kprefix, keys.ov_buf[0], klen) == 0;
		rc = 0;
	} else if (rcs[0] == -ENOENT) {
		*result = false;
		rc = 0;
	} else {
		rc = rcs[0];
	}

out_free_op:
	if (op) {
		m0_clovis_op_fini(op);
		m0_clovis_op_free(op);
	}
out_free_vals:
	m0_bufvec_free(&vals);
out_free_keys:
	m0_bufvec_free(&keys);
out:
	return rc;
}

/******************************************************************************/
/* Key Iterator */

struct m0_key_iter_priv {
	struct m0_bufvec key;
	struct m0_bufvec val;
	struct m0_clovis_op *op;
	int rcs[1];
	bool initialized;
};

_Static_assert(sizeof(struct m0_key_iter_priv) <=
	       sizeof(((struct kvstore_iter*) NULL)->priv),
	       "m0_key_iter_priv does not fit into 'priv'");

static inline
struct m0_key_iter_priv *m0_key_iter_priv(struct kvstore_iter *iter)
{
	return (void *) &iter->priv[0];
}

void m0_key_iter_fini(struct kvstore_iter *iter)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);

	if (!priv->initialized)
		goto out;

	m0_bufvec_free(&priv->key);
	m0_bufvec_free(&priv->val);

	if (priv->op) {
		m0_clovis_op_fini(priv->op);
		m0_clovis_op_free(priv->op);
	}

out:
	return;
}

bool m0_key_iter_find(struct kvstore_iter *iter, const void* prefix,
		      size_t prefix_len)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_bufvec *key = &priv->key;
	struct m0_bufvec *val = &priv->val;
	struct m0_clovis_op **op = &priv->op;
	struct m0_clovis_idx *index = iter->idx.index_priv;
	int rc;

	if (prefix_len == 0)
		rc = m0_bufvec_empty_alloc(key, 1);
	else
		rc = m0_bufvec_alloc(key, 1, prefix_len);
	if (rc != 0) {
		goto out;
	}

	rc = m0_bufvec_empty_alloc(val, 1);
	if (rc != 0) {
		goto out_free_key;
	}

	memcpy(priv->key.ov_buf[0], prefix, prefix_len);

	rc = m0_clovis_idx_op(index, M0_CLOVIS_IC_NEXT, &priv->key, &priv->val,
			      priv->rcs, 0, op);

	if (rc != 0) {
		goto out_free_val;
	}


	m0_clovis_op_launch(op, 1);
	rc = m0_clovis_op_wait(*op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);

	if (rc != 0) {
		goto out_free_op;
	}

	if (priv->rcs[0] != 0) {
		goto out_free_op;
	}

	/* release objects back to priv */
	key = NULL;
	val = NULL;
	op = NULL;
	priv->initialized = true;

out_free_op:
	if (op && *op) {
		m0_clovis_op_fini(*op);
		m0_clovis_op_free(*op);
	}

out_free_val:
	if (val)
		m0_bufvec_free(val);
out_free_key:
	if (key)
		m0_bufvec_free(key);
out:
	if (rc != 0) {
		memset(&priv, 0, sizeof(*priv));
	}

	iter->inner_rc = rc;

	return rc == 0;
}

/** Make a non-empty bufvec to be an empty bufvec.
 * Frees internal buffers (to data) inside the bufvec
 * without freeing m0_bufvec::ov_buf and m0_bufvec::ov_bec::v_count.
 */
static void m0_bufvec_free_data(struct m0_bufvec *bufvec)
{
	uint32_t i;

	assert(bufvec->ov_buf);

	for (i = 0; i < bufvec->ov_vec.v_nr; ++i) {
		m0_free(bufvec->ov_buf[i]);
		bufvec->ov_buf[i] = NULL;
	}
}

bool m0_key_iter_next(struct kvstore_iter *iter)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_clovis_idx *index = iter->idx.index_priv;
	bool can_get_next = false;

	assert(priv->initialized);

	/* Clovis API: "'vals' vector ... should contain NULLs" */
	m0_bufvec_free_data(&priv->val);

	iter->inner_rc = m0_clovis_idx_op(index, M0_CLOVIS_IC_NEXT,
					  &priv->key, &priv->val, priv->rcs,
					  M0_OIF_EXCLUDE_START_KEY,  &priv->op);

	if (iter->inner_rc != 0) {
		goto out;
	}

	m0_clovis_op_launch(&priv->op, 1);
	iter->inner_rc = m0_clovis_op_wait(priv->op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);

	if (iter->inner_rc != 0) {
		goto out;
	}

	iter->inner_rc = priv->rcs[0];

	if (iter->inner_rc == 0)
		can_get_next = true;

out:
	return can_get_next;
}

size_t m0_key_iter_get_key(struct kvstore_iter *iter, void **buf)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_bufvec *v = &priv->key;
	*buf = v->ov_buf[0];
	return v->ov_vec.v_count[0];
}

size_t m0_key_iter_get_value(struct kvstore_iter *iter, void **buf)
{
	struct m0_key_iter_priv *priv = m0_key_iter_priv(iter);
	struct m0_bufvec *v = &priv->val;
	*buf = v->ov_buf[0];
	return v->ov_vec.v_count[0];
}

/******************************************************************************/

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scro
 *  ll-step: 1
 *  End:
 */

void open_entity(struct m0_clovis_entity *entity)
{
	struct m0_clovis_op *ops[1] = {NULL};

	m0_clovis_entity_open(entity, &ops[0]);
	m0_clovis_op_launch(ops, 1);
	m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED,
					  M0_CLOVIS_OS_STABLE),
			  M0_TIME_NEVER);
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	ops[0] = NULL;
}

static void m0_indexvec_fill_extents(struct m0_indexvec *extents,
				    m0_bindex_t off,
				    m0_bcount_t block_count,
				    m0_bcount_t block_size)
{
	m0_bcount_t i;

	for (i = 0; i < block_count; i++) {
		extents->iv_index[i] = off;
		extents->iv_vec.v_count[i] = block_size;
		off += block_size;
	}
}

static int init_ctx(struct clovis_io_ctx *ioctx, off_t off,
		    int block_count, int block_size)
{
	int	     rc;
	int	     i;
	/* Allocate block_count * 4K data buffer. */
	rc = m0_bufvec_alloc(&ioctx->data, block_count, block_size);
	if (rc != 0)
		goto out_err;

	/* Allocate bufvec and indexvec for write. */
	rc = m0_bufvec_alloc(&ioctx->attr, block_count, 1);
	if (rc != 0)
		goto out_free_data;

	rc = m0_indexvec_alloc(&ioctx->ext, block_count);
	if (rc != 0)
		goto out_free_attr;

	m0_indexvec_fill_extents(&ioctx->ext, off, block_count, block_size);

	for (i = 0; i < block_count; i++) {
		/* we don't want any attributes */
		ioctx->attr.ov_vec.v_count[i] = 0;
	}

	return 0;

out_free_attr:
	m0_bufvec_free(&ioctx->attr);
out_free_data:
	m0_bufvec_free(&ioctx->data);
out_err:
	return rc;
}

int m0store_create_object(struct m0_uint128 id)
{
	int		  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	if (!my_init_done)
		m0kvs_reinit();

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	m0_clovis_entity_create(NULL, &obj.ob_entity, &ops[0]);

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		M0_TIME_NEVER);

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

int m0store_delete_object(struct m0_uint128 id)
{
	int		  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	if (!my_init_done)
		m0kvs_reinit();

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	m0_clovis_entity_delete(&obj.ob_entity, &ops[0]);

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(
		ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE),
		M0_TIME_NEVER);

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

int m0_ufid_get(struct m0_uint128 *ufid)
{
	int		  rc;

	rc = m0_ufid_next(&kvsns_ufid_generator, 1, ufid);
	if (rc != 0) {
		fprintf(stderr, "Failed to generate a ufid: %d\n", rc);
		return rc;
	}

	return 0;
}

int m0_fid_to_string(struct m0_uint128 *fid, char *fid_s)
{
	int rc;

	rc = m0_fid_print(fid_s, KVS_FID_STR_LEN, (struct m0_fid *)fid);
	if (rc < 0) {
		fprintf(stderr, "Failed to generate fid str, rc=%d", rc);
		return rc;
	}

	fprintf(stdout, "fid=%s", fid_s);
	/* rc is a buffer length, therefore it should also count '\0' */
	return rc + 1 /* '\0' */;
}

static int write_data_aligned(struct m0_uint128 id, char *buff, off_t off,
				int block_count, int block_size)
{
	int		  rc;
	int		  op_rc;
	int		  i;
	int		  nr_tries = 10;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};
	struct clovis_io_ctx    ioctx;

	if (!my_init_done)
		m0kvs_reinit();

again:
	init_ctx(&ioctx, off, block_count, block_size);

	for (i = 0; i < block_count; i++)
		memcpy(ioctx.data.ov_buf[i],
		       (char *)(buff+i*block_size),
		       block_size);

	/* Set the  bject entity we want to write */
	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	/* Create the write request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_WRITE,
			 &ioctx.ext, &ioctx.data, &ioctx.attr, 0, &ops[0]);

	/* Launch the write request*/
	m0_clovis_op_launch(ops, 1);

	/* wait */
	rc = m0_clovis_op_wait(ops[0],
			       M0_BITS(M0_CLOVIS_OS_FAILED,
			       M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	op_rc = ops[0]->op_sm.sm_rc;

	/* fini and release */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	if (op_rc == -EINVAL && nr_tries != 0) {
		nr_tries--;
		ops[0] = NULL;
		sleep(5);
		goto again;
	}

	/* Free bufvec's and indexvec's */
	m0_indexvec_free(&ioctx.ext);
	m0_bufvec_free(&ioctx.data);
	m0_bufvec_free(&ioctx.attr);

	/*
	 *    /!\    /!\    /!\    /!\
	 *
	 * As far as I have understood, MERO does the IO in full
	 * or does nothing at all, so returned size is aligned sized */
	return (block_count*block_size);
}

static int read_data_aligned(struct m0_uint128 id,
			     char *buff, off_t off,
			     int block_count, int block_size)
{
	int		     i;
	int		     rc;
	struct m0_clovis_op    *ops[1] = {NULL};
	struct m0_clovis_obj    obj;
	uint64_t		last_index;
	struct clovis_io_ctx ioctx;

	if (!my_init_done)
		m0kvs_reinit();

	rc = m0_indexvec_alloc(&ioctx.ext, block_count);
	if (rc != 0)
		return rc;

	rc = m0_bufvec_alloc(&ioctx.data, block_count, block_size);
	if (rc != 0)
		return rc;
	rc = m0_bufvec_alloc(&ioctx.attr, block_count, 1);
	if (rc != 0)
		return rc;

	last_index = off;
	for (i = 0; i < block_count; i++) {
		ioctx.ext.iv_index[i] = last_index;
		ioctx.ext.iv_vec.v_count[i] = block_size;
		last_index += block_size;

		ioctx.attr.ov_vec.v_count[i] = 0;
	}

	/* Read the requisite number of blocks from the entity */
	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	/* Create the read request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_READ,
			 &ioctx.ext, &ioctx.data, &ioctx.attr,
			 0, &ops[0]);
	assert(rc == 0);
	assert(ops[0] != NULL);
	assert(ops[0]->op_sm.sm_rc == 0);

	m0_clovis_op_launch(ops, 1);

	/* wait */
	rc = m0_clovis_op_wait(ops[0],
			       M0_BITS(M0_CLOVIS_OS_FAILED,
			       M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	assert(rc == 0);
	assert(ops[0]->op_sm.sm_state == M0_CLOVIS_OS_STABLE);
	assert(ops[0]->op_sm.sm_rc == 0);

	for (i = 0; i < block_count; i++)
		memcpy((char *)(buff + block_size*i),
		       (char *)ioctx.data.ov_buf[i],
		       ioctx.data.ov_vec.v_count[i]);


	/* fini and release */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	m0_indexvec_free(&ioctx.ext);
	m0_bufvec_free(&ioctx.data);
	m0_bufvec_free(&ioctx.attr);

	/*
	 *    /!\    /!\    /!\    /!\
	 *
	 * As far as I have understood, MERO does the IO in full
	 * or does nothing at all, so returned size is aligned sized */
	return (block_count*block_size);
}

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */


/*
 * The following functions makes random IO by blocks
 *
 */

/*
 * Those two functions compute the Upper and Lower limits
 * for the block that contains the absolution offset <x>
 * For related variables will be named Lx and Ux in the code
 *
 * ----|-----------x-------|-----
 *     Lx		  Ux
 *
 * Note: Lx and Ux are multiples of the block size
 */
static off_t lower(off_t x, size_t bs)
{
	return (x/bs)*bs;
}

static off_t upper(off_t x, size_t bs)
{
	return ((x/bs)+1)*bs;
}

/* equivalent of pwrite, but does only IO on full blocks */
ssize_t m0store_do_io(struct m0_uint128 id, enum io_type iotype,
		      off_t x, size_t len, size_t bs, char *buff)
{
	off_t Lx1, Lx2, Ux1, Ux2;
	off_t Lio, Uio, Ubond, Lbond;
	bool bprev, bnext, insider;
	off_t x1, x2;
	int bcount = 0;
	int rc;
	int delta_pos = 0;
	int delta_tmp = 0;
	ssize_t done = 0;
	char *tmpbuff;

	tmpbuff = malloc(bs);
	if (tmpbuff == NULL)
		return -ENOMEM;

	/*
	 * IO will not be considered the usual offset+len way
	 * but as segment starting from x1 to x2
	 */
	x1 = x;
	x2 = x+len;

	/* Compute Lower and Upper Limits for IO */
	Lx1 = lower(x1, bs);
	Lx2 = lower(x2, bs);
	Ux1 = upper(x1, bs);
	Ux2 = upper(x2, bs);

	/* Those flags preserve state related to the way
	 * the IO should be done.
	 * - insider is true : x1 and x2 belong to the
	 *   same block (the IO is fully inside a single block)
	 * - bprev is true : x1 is not a block limit
	 * - bnext is true : x2 is not a block limit
	 */
	bprev = false;
	bnext = false;
	insider = false;

	/* If not an "insider case", the IO can be made in 3 steps
	 * a) inside [x1,x2], find a set of contiguous aligned blocks
	 *    and do the IO on them
	 * b) if x1 is not aligned on block size, do a small IO on the
	 *    block just before the "aligned blocks"
	 * c) if x2 is not aligned in block size, do a small IO on the
	 *    block just after the "aligned blocks"
	 *
	 * Example: x1 and x2 are located so
	 *	   x <--------------- len ------------------>
	 *  ---|-----x1-----|------------|------------|-------x2--|----
	 *     Lx1	  Ux1		       Lx2	 Ux2
	 *
	 * We should (write case)
	 *   1) read block [Lx1, Ux1] and update range [x1, Ux1]
	 *     then write updated [Lx1, Ux1]
	 *   3) read block [Lx2, Ux2], update [Lx2, x2] and
	 *       then writes back updated [Lx2, Ux2]
	 */
#if 0
	printf("IO: (%lld, %llu) = [%lld, %lld]\n",
		(long long)x, (unsigned long long)len,
		(long long)x1, (long long)x2);

	printf("  Bornes: %lld < %lld < %lld ||| %lld < %lld < %lld\n",
		(long long)Lx1, (long long)x1, (long long)Ux1,
		(long long)Lx2, (long long)x2, (long long)Ux2);
#endif
	/* In the following code, the variables of interest are:
	 *  - Lio and Uio are block aligned offset that limit
	 *    the "aligned blocks IO"
	 *  - Ubond and Lbound are the Up and Low limit for the
	 *    full IO, showing every block that was touched. It is
	 *    used for debug purpose */
	if ((Lx1 == Lx2) && (Ux1 ==  Ux2)) {
		/* Insider case, x1 and x2 are so :
		 *  ---|-x1---x2----|---
		 */
		bprev = bnext = false;

		insider = true;
		Lio = Uio = 0LL;
		Ubond = Ux1;
		Lbond = Lx1;
	} else {
		/* Left side */
		if (x1 == Lx1) {
			/* Aligned on the left
			* --|------------|----
			*   x1
			*   Lio
			*   Lbond
			*/
			Lio = x1;
			bprev = false;
			Lbond = x1;
		} else {
			/* Not aligned on the left
			* --|-----x1------|----
			*		 Lio
			*   Lbond
			*/
			Lio = Ux1;
			bprev = true;
			Lbond = Lx1;
		}

		/* Right side */
		if (x2 == Lx2) {
			/* Aligned on the right
			* --|------------|----
			*		x2
			*		Uio
			*		Ubond
			*/
			Uio = x2;
			bnext = false;
			Ubond = x2;
		} else {
			/* Not aligned on the left
			* --|---------x2--|----
			*   Uio
			*		 Ubond
			*/
			Uio = Lx2;
			bnext = true;
			Ubond = Ux2;
		}
	}

	/* delta_pos is the offset position in input buffer "buff"
	 * What is before buff+delta_pos has already been done */
	delta_pos = 0;
	if (bprev) {
		/* Reads block [Lx1, Ux1] before aligned [Lio, Uio] */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx1, 1, bs);
		if (rc < 0 || rc != bs) {
			free(tmpbuff);
			return -1;
		}

		/* Update content of read block
		 * --|-----------------------x1-----------|---
		 *   Lx1				  Ux1
		 *			      WORK HERE
		 *    <----------------------><---------->
		 *	  x1-Lx1		Ux1-x1
		 */
		delta_tmp = x1 - Lx1;
		switch (iotype) {
		case IO_WRITE:
			memcpy((char *)(tmpbuff+delta_tmp),
			       buff, (Ux1 - x1));

			/* Writes block [Lx1, Ux1] once updated */
			rc = write_data_aligned(id, tmpbuff, Lx1, 1, bs);
			if (rc < 0 || rc != bs) {
				free(tmpbuff);
				return -1;
			}

			break;

		case IO_READ:
			 memcpy(buff, (char *)(tmpbuff+delta_tmp),
			       (Ux1 - x1));
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		delta_pos += Ux1 - x1;
		done += Ux1 - x1;
	}

	if (Lio != Uio) {
		/* Easy case: aligned IO on aligned limit [Lio, Uio] */
		/* If no aligned block were found, then Uio == Lio */
		bcount = (Uio - Lio)/bs;
		switch (iotype) {
		case IO_WRITE:
			rc = write_data_aligned(id, (char *)(buff + delta_pos),
						Lio, bcount, bs);

			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		case IO_READ:
			rc = read_data_aligned(id, (char *)(buff + delta_pos),
					       Lio, bcount, bs);

			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		if (rc != (bcount*bs)) {
			free(tmpbuff);
			return -1;
		}

		done += rc;
		delta_pos += done;
	}

	if (bnext) {
		/* Reads block [Lx2, Ux2] after aligned [Lio, Uio] */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx2, 1, bs);
		if (rc < 0) {
			free(tmpbuff);
			return -1;
		}

		/* Update content of read block
		 * --|---------------x2------------------|---
		 *   Lx2				 Ux2
		 *       WORK HERE
		 *    <--------------><------------------>
		 *	  x2-Lx2	   Ux2-x2
		 */
		switch (iotype) {
		case IO_WRITE:
			memcpy(tmpbuff, (char *)(buff + delta_pos),
			      (x2 - Lx2));

			/* Writes block [Lx2, Ux2] once updated */
			/* /!\ This writes extraenous ending zeros */
			rc = write_data_aligned(id, tmpbuff, Lx2, 1, bs);
			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		case IO_READ:
			memcpy((char *)(buff + delta_pos), tmpbuff,
			       (x2 - Lx2));
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		done += x2 - Lx2;
	}

	if (insider) {
		/* Insider case read/update/write */
		memset(tmpbuff, 0, bs);
		rc = read_data_aligned(id, tmpbuff, Lx1, 1, bs);
		if (rc < 0) {
			free(tmpbuff);
			return -1;
		}

		/* --|----------x1---------x2------------|---
		 *   Lx1=Lx2			     Ux1=Ux2
		 *		  UPDATE
		 *    <---------><---------->
		 *       x1-Lx1      x2-x1
		 */
		delta_tmp = x1 - Lx1;
		switch (iotype) {
		case IO_WRITE:
			memcpy((char *)(tmpbuff+delta_tmp), buff,
			       (x2 - x1));

			/* /!\ This writes extraenous ending zeros */
			rc = write_data_aligned(id, tmpbuff, Lx1, 1, bs);
			if (rc < 0) {
				free(tmpbuff);
				return -1;
			}
			break;

		case IO_READ:
			memcpy(buff, (char *)(tmpbuff+delta_tmp),
			       (x2 - x1));
			break;

		default:
			free(tmpbuff);
			return -EINVAL;
		}

		done += x2 - x1;
	}
#if 0
	printf("Complete IO : [%lld, %lld] => [%lld, %lld]\n",
		(long long)x1, (long long)x2,
		(long long)Lbond, (long long)Ubond);

	printf("End of IO : len=%llu  done=%lld\n\n",
	       (long long)len, (long long)done);
#endif
	free(tmpbuff);
	return done;
}

/** Synchronously deallocates the given vector of extents. */
static int m0_file_unmap_extents(const struct m0_uint128 *fid,
				 struct m0_indexvec *extents)
{
	int rc;
	int op_rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	if (!my_init_done)
		m0kvs_reinit();

	M0_SET0(&obj);
	m0_clovis_obj_init(&obj, &clovis_uber_realm, fid,
			   m0_clovis_layout_id(clovis_instance));

	/* Put entity in open state */
	open_entity(&obj.ob_entity);
	assert(obj.ob_entity.en_sm.sm_state == M0_CLOVIS_ES_OPEN);

	/* Create an UMMAP request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_FREE,
			 extents, NULL, NULL, 0, &ops[0]);

	/* Launch the request*/
	m0_clovis_op_launch(ops, 1);

	/* Wait for completion */
	rc = m0_clovis_op_wait(ops[0],
			       M0_BITS(M0_CLOVIS_OS_FAILED,
			       M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	op_rc = ops[0]->op_sm.sm_rc;

	/* Finalize operation */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	/* Close entity */
	m0_clovis_entity_fini(&obj.ob_entity);

	if (rc != 0) {
		fprintf(stderr, "Failed to wait for operation "
			" completion (%d).\n", rc);
	} else if (op_rc != 0) {
		fprintf(stderr, "Trunc operation has failed (%d).\n", rc);
		rc = op_rc;
	}

	return rc;
}

/** Syncronously writes zeros into the given region of an object. */
static int m0_file_zero(const struct m0_uint128 *fid,
			m0_bcount_t count,
			m0_bindex_t offset,
			m0_bcount_t bsize)
{
	void *buf = NULL;
	int rc;

	buf = calloc(1, count);
	if (buf == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	rc = m0store_do_io(*fid, IO_WRITE, offset, count, bsize, buf);
	free(buf);

	if (rc == count) {
		/* do not return positive int in case of success */
		rc = 0;
	}

out:
	return rc;
}

/* NSAL_TUNEABLE:
 * Size of data block that can be definitely UNMAP-ed by Mero
 * without generating errors at RPC layer or getting stuck somewhere
 * in the state machine.
 */
/* Default value: 5120 4K pages or 20 1MB pages or 20MB of data */
static const uint64_t m0_kvsns_trunc_data_per_request = 20 * (1 << 20);

/** Submits UNMAP requests to Clvois and waits until the data blocks
 * are actually unmapped.
 * NOTE: Mero is not able to handle large extents in the truncate operations,
 * so that we are sending only small portions of extents per request.
 */
int m0_file_unmap_aligned(struct m0_uint128 fid,
			  size_t nblocks,
			  size_t offset,
			  size_t bsize)
{
	int rc;
	struct m0_indexvec extent;
	size_t nrequests = 0;
	size_t nblk_per_req = 0;
	size_t ndata_per_req = 0;
	size_t tail_size = 0;
	size_t i;

	M0_DASSERT(bsize != 0);
	M0_DASSERT(bsize % 2 == 0);
	M0_DASSERT(offset % bsize == 0);
	/* offset + nblocks * bsize == count, count <= SIZE_MAX */
	M0_DASSERT(SIZE_MAX / bsize >= (offset / bsize) + nblocks);

	if (nblocks == 0) {
		fprintf(stdout, "Nothing to unmap.\n");
		goto out;
	}

	ndata_per_req = lower(m0_kvsns_trunc_data_per_request, bsize);
	nblk_per_req = ndata_per_req / bsize;
	nrequests = nblocks / nblk_per_req;
	tail_size = (nblocks * bsize) - (nrequests * ndata_per_req);

	M0_DASSERT(ergo(nblocks * bsize - offset < ndata_per_req,
			   nrequests == 0));

	rc = m0_indexvec_alloc(&extent, 1);
	if (rc != 0) {
		goto out;
	}

	/* Synchonously deallocate a batch of extents (ndata_per_req in each
	 * extent) and then synchronously deallocate the tail
	 * which is not aligned with the ndata_per_req value.
	 */

	for (i = 0; i < nrequests; i++) {
		fprintf(stdout, "De-allocating large extent[%d]: off=%llu, size=%d, "
			  "done=%.02f%%\n",
			  (int) i,
			  (unsigned long long) offset,
			  (int) ndata_per_req,
			  (((float) i * ndata_per_req) / (nblocks * bsize)) * 100);

		extent.iv_index[0] = offset;
		extent.iv_vec.v_count[0] = ndata_per_req;

		rc = m0_file_unmap_extents(&fid, &extent);
		if (rc != 0) {
			fprintf(stdout, "Failed to unmap the extent: %llu, %llu.\n",
				(unsigned long long) offset,
				(unsigned long long) ndata_per_req);
			goto out_free_extent;
		}

		offset += ndata_per_req;
	}

	if (tail_size) {
		fprintf(stdout, "De-allocating tail extent: off=%llu, size=%d\n",
			  (unsigned long long) offset,
			  (int) tail_size);
		extent.iv_index[0] = offset;
		extent.iv_vec.v_count[0] = tail_size;
		rc = m0_file_unmap_extents(&fid, &extent);
		if (rc != 0) {
			goto out_free_extent;
		}
	}

out_free_extent:
	m0_indexvec_free(&extent);
out:
	return rc;
}

int m0_file_unmap(struct m0_uint128 fid, size_t count, off_t offset)
{
	int rc;
	int bsize; /* Clovis block size */
	size_t nblocks; /* n blocks to be deallocated */
	size_t aligned_off; /* Rounded up offset */

	bsize = m0store_get_bsize(fid);

	M0_DASSERT(bsize > 0);
	M0_DASSERT(bsize % 2 == 0);

	/* FIXME:EOS-1819: should we support count/offset  more than 7EB? */
	M0_DASSERT(count < INT64_MAX);
	M0_DASSERT(offset < INT64_MAX);
	M0_DASSERT(count != 0);

	/* adjust to the very first byte of the nearest (from right) page */
	aligned_off = m0_round_up(offset, bsize);
	/* cut out the left unaligned part from the whole len,
	 * round it up and then count the amount of blocks */
	nblocks = m0_round_up((count + offset) - aligned_off, bsize) / bsize;

	/* A special case where the caller wants to free a small range
	 * which cannot be de-allocated.
	 */
	if (m0_round_up(offset, bsize) == m0_round_up(offset + count, bsize)) {
		fprintf(stdout, "the range [%llu,%llu] is inside a single block %llu\n",
			  (unsigned long long) offset,
			  (unsigned long long) offset + count,
			  (unsigned long long) aligned_off);
		/* FIXME:EOS-1819: We should write zeros here because if
		 * the client would like to increase the file size back,
		 * the extented space must be read as zeros. */
		rc = m0_file_zero(&fid, count, offset, bsize);
		/* no need to do actual UNMAP */
		goto out;
	}

	/* A special case where the left edge is not aligned with the block
	 * size. The unaligned space must be zeroed.
	 */
	if (offset != aligned_off) {
		M0_DASSERT(offset < aligned_off);
		fprintf(stdout, "Non-freed range=[%llu, %llu]\n",
			  (unsigned long long) offset,
			  (unsigned long long) aligned_off);
		/* FIXME: the same case: zero the range which won't be unmapped */
		rc = m0_file_zero(&fid, aligned_off - offset, offset, bsize);
		if (rc != 0) {
			goto out;
		}
		/* now unmap the aligned pages */
	}

	rc = m0_file_unmap_aligned(fid, nblocks, aligned_off, bsize);

out:
	return rc;
}

ssize_t m0store_get_bsize(struct m0_uint128 id)
{
	return m0_clovis_obj_layout_id_to_unit_size(
			m0_clovis_layout_id(clovis_instance));
}

void *m0kvs_alloc(uint64_t size)
{
	return m0_alloc(size);
}

void m0kvs_free(void *ptr)
{
	return m0_free(ptr);
}
