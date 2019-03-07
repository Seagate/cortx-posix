#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fnmatch.h>
#include <lib/semaphore.h>
#include <lib/trace.h>
#include <lib/memory.h>
#include <lib/mutex.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

#define CONF_FILE    "./conf"
#define BUFF_LEN     256
#define MAXC0RC      5
#define BLOCKSIZE    4096
#define KLEN         256
#define VLEN         256
#define LIST_TRUNK   10
#define CLOVIS_MAX_BLOCK_COUNT  (1)

static struct m0_clovis_config    clovis_conf;
static struct m0_clovis          *clovis_instance = NULL;
static struct m0_clovis_container clovis_container;
static struct m0_clovis_realm     clovis_uber_realm;
static struct m0_clovis_config    clovis_conf;
static struct m0_idx_dix_config   dix_conf;

static struct m0_fid              ifid;
static struct m0_clovis_idx       cidx;

static char    c0rc[10][BUFF_LEN];

struct m0_clovis_op_ctx {
	struct m0_mutex      coc_mlock;
	struct m0_semaphore  coc_sem;
	uint32_t             coc_total_blocks;
	uint32_t             coc_blocks_written;
} op_ctx;

struct m0_clovis_vec_ctx {
	struct m0_clovis_op_ctx   *cvc_ctx;
	struct m0_indexvec         cvc_ext;
	struct m0_bufvec           cvc_data;
	struct m0_bufvec           cvc_attr;
	struct m0_clovis_obj       cvc_obj;
};

typedef struct m0_item {
	int offset;
	char str[KLEN];
} m0_item_t;

typedef struct m0_kvlist {
	char pattern[KLEN];
	m0_item_t *content;
	size_t size;
} m0_kvlist_t;

typedef bool (*get_list_cb)(char *k, void *arg);

/*
 ******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 ******************************************************************************
 */
static char *trim(char *str);
static int open_entity(struct m0_clovis_entity *entity);
static int create_object(struct m0_uint128 id);
static int read_data_from_file(FILE *fp, struct m0_bufvec *data, int bsz);
static int clovis_write_vec_init(struct m0_clovis_vec_ctx *v_ctx,
								 uint32_t blk_count, uint32_t block_size);
static int clovis_vec_alloc(struct m0_clovis_vec_ctx **v_ctx, uint32_t blks);
static int clovis_op_alloc(struct m0_clovis_op ***op, uint32_t blks);
static int write_data_to_object_async(struct m0_uint128 id,
									  struct m0_clovis_vec_ctx *v_ctx,
									  struct m0_clovis_op **op);
static void op_ctx_fini(struct m0_clovis_op_ctx *op_ctx);
static void op_ctx_init (struct m0_clovis_op_ctx *op_ctx, uint32_t block_count);
static void clovis_write_vec_free(struct m0_clovis_vec_ctx *v_ctx);
static void clovis_write_failed_cb(struct m0_clovis_op *op);
static void clovis_write_stable_cb(struct m0_clovis_op *op);
static void clovis_write_executed_cb(struct m0_clovis_op *op);

static int m0_op_kvs(enum m0_clovis_idx_opcode opcode, struct m0_bufvec *key,
		      struct m0_bufvec *val);

static int m0kvs_set(char *k, char *v);
static int m0kvs_get(char *k, char *v);
static int m0write(char *src, int64_t idh, int64_t idl, int block_count);

/*
 * trim()
 * trim leading/trailing white spaces.
 */
static char *trim(char *str)
{
	char *end;

	/* null */
	if (!str) return str;

	/* trim leading spaces */
	while (isspace((unsigned char)*str)) str++;

	/* all spaces */
	if (*str == 0) return str;

	/* trim trailing spaces, and */
	/* write new null terminator */
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	*(end + 1) = 0;

	/* success */
	return str;
}


/*
 * clovis_write_vec_init()
 * initializes the buf and index vectors
 */
static int clovis_write_vec_init(struct m0_clovis_vec_ctx *v_ctx,
								 uint32_t blk_count, uint32_t block_size)
{
	int rc = 0;

	/* Allocate block_count * 4K data buffer. */
	rc = m0_bufvec_alloc(&v_ctx->cvc_data, blk_count, block_size);
	if (rc != 0)
		return rc;

	/* Allocate bufvec and indexvec for write. */
	rc = m0_bufvec_alloc(&v_ctx->cvc_attr, blk_count, 1);
	if (rc != 0)  {
		m0_bufvec_free(&v_ctx->cvc_data);
		return rc;
	}

	rc = m0_indexvec_alloc(&v_ctx->cvc_ext, blk_count);
	if (rc != 0) {
		m0_bufvec_free(&v_ctx->cvc_data);
		m0_bufvec_free(&v_ctx->cvc_attr);
		return rc;
	}
	return rc;
}

static void clovis_write_vec_free(struct m0_clovis_vec_ctx *v_ctx)
{
	/* Free bufvec's and indexvec's */
	m0_indexvec_free(&v_ctx->cvc_ext);
	m0_bufvec_free(&v_ctx->cvc_data);
	m0_bufvec_free(&v_ctx->cvc_attr);
}

int init(int idx)
{
	int   i;
	int   rc;
	FILE *fp;
	char *str = NULL;
	char  buf[256], tmpfid[256];
	char* filename = CONF_FILE;

	/* read conf file */
	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr,"error! could not open resource file %s\n", filename);
		return 1;
	}

	/* move fp */
	i = 0;
	while ((i != idx * MAXC0RC) && (fgets(buf, BUFF_LEN, fp) != NULL)) {
		str = trim(buf);
		if (str[0] == '#') continue;     /* ignore comments */
		if (strlen(str) == 0) continue;  /* ignore empty space */
		i++;
	}

	/* read c0rc */
	i = 0;
	while (fgets(buf, BUFF_LEN, fp) != NULL) {

#if DEBUG
	fprintf(stderr,"rd:->%s<-", buf);
	fprintf(stderr,"\n");
#endif

	str = trim(buf);
	if (str[0] == '#') continue;		/* ignore comments */
	if (strlen(str) == 0) continue;		/* ignore empty space */

	memset(c0rc[i], 0x00, BUFF_LEN);
	strncpy(c0rc[i], str, BUFF_LEN);

#if DEBUG
	fprintf(stderr,"wr:->%s<-", c0rc[i]);
	fprintf(stderr,"\n");
#endif

	i++;
	if (i == MAXC0RC) break;
	}
	fclose(fp);

	/* idx check */
	if (i != MAXC0RC) {
		fprintf(stderr,"error! [[ %d ]] wrong resource index.\n", idx);
		return 2;
	}

	clovis_conf.cc_is_oostore            = true;
	clovis_conf.cc_is_read_verify        = false;
	clovis_conf.cc_local_addr            = c0rc[0];
	clovis_conf.cc_ha_addr               = c0rc[1];
	clovis_conf.cc_profile               = c0rc[2];
	clovis_conf.cc_process_fid           = c0rc[3];
	clovis_conf.cc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
	clovis_conf.cc_max_rpc_msg_size      = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
	clovis_conf.cc_layout_id             = 9;

	/* IDX_MERO */
	clovis_conf.cc_idx_service_id   = M0_CLOVIS_IDX_DIX;
	dix_conf.kc_create_meta = false;
	clovis_conf.cc_idx_service_conf = &dix_conf;

	#if DEBUG
	fprintf(stderr,"\n---\n");
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_local_addr);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_ha_addr);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_profile);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_process_fid);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_idx_service_conf);
	fprintf(stderr,"\n---\n");
	#endif

	/* clovis instance */
	rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);
	if (rc != 0) {
		fprintf(stderr,"failed to initilise Clovis\n");
		return rc;
	}

	/* And finally, clovis root realm */
	m0_clovis_container_init(&clovis_container,
				 NULL, &M0_CLOVIS_UBER_REALM,
				 clovis_instance);
	rc = clovis_container.co_realm.re_entity.en_sm.sm_rc;
	if (rc != 0) {
		fprintf(stderr,"failed to open uber realm\n");
		return rc;
	}

	/* success */
	clovis_uber_realm = clovis_container.co_realm;

	/* Get fid from config parameter */
	memset(&ifid, 0, sizeof(struct m0_fid));
	rc = m0_fid_sscanf(c0rc[4], &ifid);
	if (rc != 0) {
		fprintf(stderr, "Failed to read ifid value from conf\n");
		return rc;
	}

	rc = m0_fid_print(tmpfid, 256, &ifid);
	if (rc < 0) {
		fprintf(stderr, "Failed to read ifid value from conf\n");
		return rc;
	}

	m0_clovis_idx_init(&cidx, &clovis_container.co_realm, (struct m0_uint128 *)&ifid);

	return 0;
}


/*
 * open_entity()
 * open clovis entity.
 */
static int open_entity(struct m0_clovis_entity *entity)
{
	int                  rc;
	struct m0_clovis_op *ops[1] = {NULL};

	m0_clovis_entity_open(entity, &ops[0]);
	m0_clovis_op_launch(ops, 1);
	m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED,
									  M0_CLOVIS_OS_STABLE),
					  M0_TIME_NEVER);
	rc = m0_clovis_rc(ops[0]);
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	ops[0] = NULL;

	return rc;
}

static void clovis_write_executed_cb(struct m0_clovis_op *op)
{
	//m0_console_printf("Write operation is executing\n");
}

/*
 * clovis_write_stable_cb()
 * callback for stable state of clovis op
 */
static void clovis_write_stable_cb(struct m0_clovis_op *op)
{
	uint32_t                *blks;
	uint32_t                 tot_blks;
	struct m0_semaphore     *sem;
	struct m0_mutex         *lock;
	struct m0_clovis_op_ctx *o_ctx;

	o_ctx = ((struct m0_clovis_vec_ctx *)op->op_datum)->cvc_ctx;
	blks = &o_ctx->coc_blocks_written;
	tot_blks = o_ctx->coc_total_blocks;
	sem = &o_ctx->coc_sem;
	lock = &o_ctx->coc_mlock;
	m0_console_printf("Write operation is STABLE for blocks %d", *blks);
	m0_mutex_lock(lock);
	*blks += CLOVIS_MAX_BLOCK_COUNT;
	m0_mutex_unlock(lock);
	m0_console_printf(" - %d\n", *blks);
	printf("(Callback) The thread id is %u \n", (unsigned int) syscall( __NR_gettid ));
	if (*blks >= tot_blks)
		m0_semaphore_up(sem);
}

/*
 * clovis_write_failed_cb()
 * callback for failed state of clovis op
 */
static void clovis_write_failed_cb(struct m0_clovis_op *op)
{
	uint32_t                 *blks;
	uint32_t                  tot_blks;
	struct m0_semaphore      *sem;
	struct m0_mutex          *lock;
	struct m0_clovis_op_ctx  *o_ctx;

	o_ctx = ((struct m0_clovis_vec_ctx *)op->op_datum)->cvc_ctx;
	blks = &o_ctx->coc_blocks_written;
	tot_blks = o_ctx->coc_total_blocks;
	sem = &o_ctx->coc_sem;
	lock = &o_ctx->coc_mlock;
	m0_console_printf("Write operation FAILED for blocks %d", *blks);
	m0_mutex_lock(lock);
	*blks += CLOVIS_MAX_BLOCK_COUNT;
	m0_mutex_unlock(lock);
	m0_console_printf(" - %d\n", *blks);
	if (*blks >= tot_blks)
		m0_semaphore_up(sem);
}

static int clovis_op_alloc(struct m0_clovis_op ***op, uint32_t blks)
{
	int      i;
	uint32_t no_of_ops = blks / CLOVIS_MAX_BLOCK_COUNT;
	if (blks % CLOVIS_MAX_BLOCK_COUNT)
		no_of_ops++;
	M0_ALLOC_ARR(*op, no_of_ops);
	for (i = 0; i < no_of_ops; i++)
		(*op)[i] = NULL;
	if ( *op == NULL)
		return -1;
	return 0;
}

static int clovis_vec_alloc(struct m0_clovis_vec_ctx **v_ctx, uint32_t blks)
{
	int      i;
	uint32_t no_of_ops = blks / CLOVIS_MAX_BLOCK_COUNT;
	if (blks % CLOVIS_MAX_BLOCK_COUNT)
		no_of_ops++;
	M0_ALLOC_ARR(*v_ctx, no_of_ops);
	for (i = 0; i < no_of_ops; i++)
		((*v_ctx)[i]).cvc_ctx = &op_ctx;
	if (*v_ctx == NULL)
		return -1;
	return 0;
}

static void op_ctx_init (struct m0_clovis_op_ctx *op_ctx, uint32_t block_count)
{
	m0_mutex_init(&op_ctx->coc_mlock);
	m0_semaphore_init(&op_ctx->coc_sem, 0);
	op_ctx->coc_total_blocks = block_count;
	op_ctx->coc_blocks_written = 0;
}

static void op_ctx_fini(struct m0_clovis_op_ctx *op_ctx)
{
	m0_mutex_fini(&op_ctx->coc_mlock);
	m0_semaphore_fini(&op_ctx->coc_sem);
	op_ctx->coc_total_blocks = 0;
	op_ctx->coc_blocks_written = 0;
}

/*
 * create_object()
 * create clovis object.
 */
static int create_object(struct m0_uint128 id)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	rc = open_entity(&obj.ob_entity);
	if (!(rc < 0)) {
		fprintf(stderr,"error! [%d]\n", rc);
		fprintf(stderr,"object already exists\n");
		return 1;
	}

	/*
	 * use default pool by default:
	 * struct m0_fid *pool = NULL
	 */
	m0_clovis_entity_create(NULL, &obj.ob_entity, &ops[0]);

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED,
										   M0_CLOVIS_OS_STABLE),
						   m0_time_from_now(3,0));

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return 0;
}

/*
 * write_data_to_object_async()
 * writes to and object in async manner
 */
static int write_data_to_object_async(struct m0_uint128 id,
									  struct m0_clovis_vec_ctx *v_ctx,
									  struct m0_clovis_op **op)
{
	int                       rc = 0;
	struct m0_clovis_op      *ops[1] = {NULL};
	struct m0_clovis_op_ops   op_ops;

	memset(&v_ctx->cvc_obj, 0, sizeof(struct m0_clovis_obj));
	/* Set the object entity we want to write */
	m0_clovis_obj_init(&v_ctx->cvc_obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	rc = open_entity(&v_ctx->cvc_obj.ob_entity);

	/* Create the write request */
	m0_clovis_obj_op(&v_ctx->cvc_obj, M0_CLOVIS_OC_WRITE,
			 &v_ctx->cvc_ext, &v_ctx->cvc_data,
			 &v_ctx->cvc_attr, 0, op);

	/* Attaching Callbacks */
	op_ops.oop_executed = clovis_write_executed_cb;
	op_ops.oop_stable = clovis_write_stable_cb;
	op_ops.oop_failed = clovis_write_failed_cb;
	m0_clovis_op_setup(*op, &op_ops, 0);

	(*op)->op_datum = v_ctx;
	ops[0] = *op;

	printf("(async req) The thread id is %u \n", (unsigned int) syscall( __NR_gettid ));
	/* Launch the write request */
	m0_clovis_op_launch(ops, 1);

	return rc;
}

/*
 * read_data_from_file()
 * reads data from file and populates buffer.
 */
static int read_data_from_file(FILE *fp, struct m0_bufvec *data, int bsz)
{
	int i;
	int rc;
	int nr_blocks;

	nr_blocks = data->ov_vec.v_nr;
	for (i = 0; i < nr_blocks; i++) {
		rc = fread(data->ov_buf[i], bsz, 1, fp);
		if (rc != 1)
			break;

		if (feof(fp))
			break;
	}

	return i;
}


static int m0_op_kvs(enum m0_clovis_idx_opcode opcode,
		      struct m0_bufvec *key, struct m0_bufvec *val)
{
	struct m0_clovis_op	 *op = NULL;
	int rcs[1];
	int rc;

	rc = m0_clovis_idx_op(&cidx, opcode, key, val,
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

bool get_list_cb_size(char *k, void *arg)
{
	int size;

	memcpy((char *)&size, (char *)arg, sizeof(int));
	size += 1;
	memcpy((char *)arg, (char *)&size, sizeof(int));

	return true;
}

bool populate_list(char *k, void *arg)
{
	m0_kvlist_t *list;
	m0_item_t *item;
	m0_item_t *content;

	list = (m0_kvlist_t *)arg;

	if (!list)
		return false;

	list->size +=1;
	content = list->content;

	list->content = realloc(content, list->size*sizeof(m0_item_t));
	if (list->content == NULL)
		return false;

	item = &list->content[list->size - 1];

	strncpy(item->str, k, KLEN);
	item->offset = list->size -1;

	return true;
}

static int m0_pattern_kvs(char *k, char *pattern,
			  get_list_cb cb, void *arg_cb)
{
	struct m0_bufvec       keys;
	struct m0_bufvec       vals;
	struct m0_clovis_op       *op = NULL;
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
		rc = m0_bufvec_alloc(&keys, 1, KLEN) ?:
			m0_bufvec_alloc(&vals, 1, VLEN);
		if (rc != 0)
			return rc;

		keys.ov_buf[0] = m0_alloc(strnlen(myk, KLEN)+1);
		keys.ov_vec.v_count[0] = strnlen(myk, KLEN)+1;
		strcpy(keys.ov_buf[0], myk);

		rc = m0_clovis_idx_op(&cidx, M0_CLOVIS_IC_NEXT, &keys, &vals,
				      rcs, flags,  &op);
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
#if 0
			if (op) {
				m0_clovis_op_fini(op);
				m0_free0(&op);
			}
#endif
			return rc;
		}

		if (rcs[0] == -ENOENT) {
			m0_bufvec_free(&keys);
			m0_bufvec_free(&vals);
#if 0
			if (op) {
				m0_clovis_op_fini(op);
				m0_free0(&op);
			}
#endif

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
#if 0
		if (op) {
			m0_clovis_op_fini(op);
			m0_free0(&op);
		}
#endif

	} while (!stop);

	return size;
}

static int m0_get_list_size(char *pattern)
{
	char initk[KLEN];
	int size = 0;
	int rc;

	strncpy(initk, pattern, KLEN);
	initk[strnlen(pattern, KLEN)-1] = '\0';

	rc = m0_pattern_kvs(initk, pattern,
			    get_list_cb_size, &size);
	if (rc < 0)
		return rc;

	return size;
}

static int m0_fetch_list(char *pattern, m0_kvlist_t *list)
{
	char initk[KLEN];

	if (!pattern || !list)
		return -EINVAL;

	strncpy(initk, pattern, KLEN);
	initk[strnlen(pattern, KLEN)-1] = '\0';

	return  m0_pattern_kvs(initk, pattern,
			       populate_list, list);
}

/** @todo: too many strncpy and mallocs, this should be optimized */
static int m0_get_list(m0_kvlist_t *list, int start, int *size,
		       m0_item_t *items)
{
	int i;

	if (list->size < (start + *size))
		*size = list->size - start;


	for (i = start; i < start + *size ; i++) {
		items[i-start].offset = i;
		strncpy(items[i-start].str,
			list->content[i].str, KLEN);
	}

	return 0;
}

int cleanup(void)
{
	m0_clovis_fini(clovis_instance, true);
	return 0;
}

static int m0kvs_set(char *k, char *v)
{
	int rc = 0;
	struct m0_bufvec    key;
	struct m0_bufvec    val;
	size_t              klen;
	size_t              vlen;

	klen = strnlen(k, KLEN)+1;
	vlen = strnlen(v, VLEN)+1;

	rc = m0_bufvec_alloc(&key, 1, klen) ?:
		m0_bufvec_alloc(&val, 1, vlen);
	if (rc)
		goto out;

	memcpy(key.ov_buf[0], k, klen);
	memcpy(val.ov_buf[0], v, vlen);

	rc = m0_op_kvs(M0_CLOVIS_IC_PUT, &key, &val);
out:
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;

}

static int m0kvs_get(char *k, char *v)
{
	int rc;
	struct m0_bufvec     key;
	struct m0_bufvec     val;
	size_t               klen;
	size_t               vlen;

	klen = strnlen(k, KLEN)+1;
	vlen = VLEN;

	rc = m0_bufvec_alloc(&key, 1, klen) ?:
		m0_bufvec_empty_alloc(&val, 1);

	memcpy(key.ov_buf[0], k, klen);
	memset(v, 0, vlen);

	rc = m0_op_kvs(M0_CLOVIS_IC_GET, &key, &val);
	if (rc)
		goto out;

	vlen = (size_t)val.ov_vec.v_count[0];
	memcpy(v, (char *)val.ov_buf[0], vlen);

out:
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;
}

static int m0kvs_list(char *k, char *v)
{
	int rc, i;
	int maxsize = 0;
	int offset = 0;
	int size = LIST_TRUNK;
	m0_item_t items[LIST_TRUNK];
	m0_kvlist_t list;

	list.size = 0;
	list.content = NULL;

	rc = m0_get_list_size(k);
	if (rc < 0) {
		fprintf(stderr, "m0_get_list_size: err=%d\n", rc);
		exit(1);
	}
	maxsize = rc;
	printf("m0_get_list_size: found %d items\n", rc);

	rc = m0_fetch_list(k, &list);
	if (rc != 0) {
		fprintf(stderr, "m0_fetch_list: err=%d\n", rc);
		exit(-rc);
	}

	while (offset < maxsize) {
		size = LIST_TRUNK;
		rc = m0_get_list(&list, offset, &size, items);
		if (rc < 0) {
			fprintf(stderr, "m0_get_list: err=%d\n", rc);
			exit(-rc);
		}
		for (i = 0; i < size ; i++)
			printf("==> %d %s\n", offset+i, items[i].str);

		offset += size;
	}


	return rc;
}

static int m0write(char *src, int64_t idh, int64_t idl, int block_count)
{
	int                        i, rc, block_size = BLOCKSIZE;
	uint64_t                   last_index;
	struct m0_uint128          id;
	struct m0_clovis_vec_ctx  *v_ctx;
	struct m0_clovis_op      **op;
	uint32_t                   blk_count;
	uint32_t                   op_index;
	FILE                      *fp;

	/* Open source file */
	fp = fopen(src, "rb");
	if (fp == NULL)
		return -1;

	/* ids */
	id.u_hi = idh;
	id.u_lo = idl;

	/* Create the object */
	rc = create_object(id);
	if (rc != 0) {
		fclose(fp);
		return rc;
	}

	/* Allocating memory for array of operation pointers */
	rc = clovis_op_alloc(&op, block_count);
	if (rc != 0) {
		fclose(fp);
		return rc;
	}

	/* Initializing operation context */
	op_ctx_init(&op_ctx, block_count);
	rc = clovis_vec_alloc(&v_ctx, block_count);
	if (rc != 0) {
		fclose(fp);
		free(op);
		return rc;
	}

	last_index = 0;
	op_index = 0;

	while (block_count > 0) {

		blk_count = (block_count > CLOVIS_MAX_BLOCK_COUNT)?
			CLOVIS_MAX_BLOCK_COUNT:block_count;
		rc = clovis_write_vec_init(&v_ctx[op_index], blk_count,
					   block_size);
		if (rc != 0) {
			fclose(fp);
			free(op);
			free(v_ctx);
			return rc;
		}
		/*
		* Prepare indexvec for write: <clovis_block_count> from the
		* beginning of the object.
		*/
		for (i = 0; i < blk_count; i++) {
			v_ctx[op_index].cvc_ext.iv_index[i] = last_index;
			v_ctx[op_index].cvc_ext.iv_vec.v_count[i] = block_size;
			last_index += block_size;

			/* we don't want any attributes */
			v_ctx[op_index].cvc_attr.ov_vec.v_count[i] = 0;
		}

		/* Read data from source file. */
		rc = read_data_from_file(fp, &v_ctx[op_index].cvc_data, block_size);
		assert(rc == blk_count);

		/* Copy data to the object*/
		rc = write_data_to_object_async(id, &v_ctx[op_index],
										&op[op_index]);
		if (rc != 0) {
			fprintf(stderr, "Writing to object failed!\n");
			goto fail;
		}
		op_index++;
		block_count -= blk_count;

	}

	m0_semaphore_down(&op_ctx.coc_sem);

fail:
	for (i = 0; i< op_index; i++) {
		m0_clovis_op_fini(op[i]);
		m0_clovis_op_free(op[i]);
		m0_clovis_entity_fini(&v_ctx[i].cvc_obj.ob_entity);
		clovis_write_vec_free(&v_ctx[i]);
	}

	/* Finalizing operation context */
	op_ctx_fini(&op_ctx);
	free(op);
	free(v_ctx);
	fclose(fp);
	return rc;
}

int process_command(char *cmd)
{
	int rc = 0;
	char oper[BUFF_LEN];

	printf("Command: %s", cmd);
	sscanf(cmd, "%s", oper);
	cmd += strlen(oper);

	if (strcmp(oper, "m0write") == 0) {
		int64_t  idh;    /* object id high       */
		int64_t  idl;    /* object id low        */
		int      block_count;
		char     src[BUFF_LEN];

		sscanf (cmd,"%s %lld %lld %d", src, (long long int*)&idh, (long long int*)&idl, &block_count);
		rc = m0write(src, idh, idl, block_count);
		if (rc != 0) {
			fprintf(stderr,"m0write command failed.\n");
			return rc;
		}
	} else if (strcmp(oper, "m0kvs_set") == 0) {
		char     key[BUFF_LEN], val[BUFF_LEN];

		sscanf (cmd,"%s %s", key, val);
		rc = m0kvs_set(key, val);
		if (rc != 0) {
			fprintf(stderr,"m0kvs_set command failed.\n");
			return rc;
		}
	} else if (strcmp(oper, "m0kvs_get") == 0) {
		char     key[BUFF_LEN], val[BUFF_LEN];

		sscanf (cmd,"%s", key);
		rc = m0kvs_get(key, val);
		if (rc != 0) {
			fprintf(stderr,"m0kvs_get command failed.\n");
			return rc;
		}
		printf("====> #%s# -> #%s#\n", key, val);
	} else if (strcmp(oper, "m0kvs_list") == 0) {
		char     key[BUFF_LEN], val[BUFF_LEN];

		sscanf (cmd,"%s*", key);
		rc = m0kvs_list(key, val);
		if (rc != 0) {
			fprintf(stderr,"m0kvs_list command failed.\n");
			return rc;
		}
	}

	return rc;
}



int main (int argc, char *argv[])
{
	char *cmd_file;
	int arg = 0, rc = 0;

	if (argc < 2) {
		printf ("usage: %s <cmd_file> \n", argv[0]);
		return 0;
	}

	/* initialize resources */
	if (init(0) != 0) {
		fprintf(stderr,"error! clovis initialization failed.\n");
		return -2;
	}

	cmd_file = argv[++arg];
	FILE *cmd_fptr = fopen(cmd_file, "r");
	if (cmd_fptr != NULL) {
		char line[128]; /* or other suitable maximum line size */
		while (fgets (line, sizeof(line), cmd_fptr) != NULL) {
			rc = process_command(line);
			if (rc != 0) {
				fprintf(stderr,"Command failed.\n");
				return -2;
			}
		}
		fclose (cmd_fptr);
	}
	else {
		perror (cmd_file); /* why didn't the file open? */
	}

	/* free resources*/
	cleanup();

	return 0;
}
