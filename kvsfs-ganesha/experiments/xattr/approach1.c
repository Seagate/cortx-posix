/* -*- C -*- */
/*
 * COPYRIGHT 2014 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE LLC,
 * ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.xyratex.com/contact
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "c0appz.h"
#include "helpers/helpers.h"
#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"
#include "lib/thread.h"
#include <json-c/json.h>
#include <sys/time.h>	
#include <unistd.h>
#include <stdbool.h> 
#define KLEN 256
#define VLEN 256
#define VALINPUT 512
#define MAXVAL 70000 
#define CNT 100 

struct kvsns_xattr{
	unsigned long long int ino;
	char type;
	char name[256];
}__attribute((packed));


#define XATTR_KEY_INIT(key, ino2, xname)	\
{						\
        key->ino = ino2;			\
	key->type = '7';			\
	memset(key->name, 0, 256);		\
        memcpy(key->name, xname, strlen(xname));\
}

static struct m0_fid ifid;
static struct m0_ufid_generator kvsns_ufid_generator;
static struct m0_clovis_idx idx;

static int *rcs_alloc(int count)
{
        int  i;
        int *rcs;

        M0_ALLOC_ARR(rcs, count);
        for (i = 0; i < count; i++)
                rcs[i] = 0xdb;
        return rcs;
}

void timer(struct timeval start1, struct timeval end1, char *msg)
{
	long mtime, secs, usecs;
	secs  = end1.tv_sec  - start1.tv_sec;
	usecs = end1.tv_usec - start1.tv_usec;
	mtime = ((secs) * 1000 + usecs/1000.0) + 0.5;
 	printf("Elapsed time for %s: %ld millisecs\n", msg, mtime);

}

static int m0_op_kvs(enum m0_clovis_idx_opcode opcode, struct m0_bufvec *key, struct m0_bufvec *val)
{
	struct m0_clovis_op	 *op = NULL;
	int *rcs;
	int rc;

	struct m0_clovis_idx     *index = NULL;

	index = &idx;
	rcs = rcs_alloc(CNT);

	rc = m0_clovis_idx_op(index, opcode, key, val, rcs, M0_OIF_OVERWRITE, &op);

	if (rc)
	{
               printf("\nerror(%d): m0_clovis_idx_op", rc); 
	       return rc;
	}
	m0_clovis_op_launch(&op, 1);

	rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
			       M0_TIME_NEVER);
	
	if (rc)
	{
		printf("\nerror(%d): m0_clovis_op_wait", rc);
		goto out;
	}
	/* Check rcs array even if op is succesful */
//	rc = rcs[0];
	int j;
	for (j = 0; rc == 0 && j < CNT; j++)
		rc = rcs[j];

	if (rc)
	{
		printf("\nerror(%d):rcs array", rc);
		goto out;
	}

out:
	m0_clovis_op_fini(op);
	/* it seems like 0_free(&op) is not needed */
	return rc;
}

int delete_batch(char *k1, char *ino)
{
	struct kvsns_xattr *xkey[CNT];
	size_t klen;
	int rc, i;
	struct m0_bufvec key;

	unsigned long long int ino2;
	ino2 = atoll(ino);

	char tmpkey[256];
        struct timeval start1, end1;

        klen = sizeof(struct kvsns_xattr);

        for (i = 0; i < CNT; i++)
        {
                xkey[i] = calloc(1, sizeof( struct kvsns_xattr));
                snprintf(tmpkey, 256, "%s_%d", k1, i);
                XATTR_KEY_INIT(xkey[i], ino2, tmpkey);
	}
	
        gettimeofday(&start1, NULL);
	
	rc = m0_bufvec_alloc(&key, CNT, klen);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_alloc", rc);
		goto out;
	}
	for (i = 0; i < CNT; i++)
        	memcpy(key.ov_buf[i], xkey[i], klen);

	rc = m0_op_kvs(M0_CLOVIS_IC_DEL, &key, NULL);
	if (rc)
        {
		printf("\nerror(%d): m0_op_kvs", rc);
		goto out;
	}

  out:
       	for (i = 0; i < CNT; i ++)        	
		free(xkey[i]);
	
	m0_bufvec_free(&key);
        gettimeofday(&end1, NULL);
	timer(start1, end1, "del 100 batch  keys  (nfs)");
	
	return rc;
}

int get_keyval(char *name, char *v, unsigned long long int ino2)
{
	struct kvsns_xattr *xkey = NULL;

	size_t klen;
	size_t vlen = 256;

	struct m0_bufvec key;
	struct m0_bufvec val;
	int rc;

	xkey = calloc(1, sizeof(*xkey));

	XATTR_KEY_INIT(xkey, ino2, name);

	klen = sizeof(*xkey);
	rc = m0_bufvec_alloc(&key, 1, klen);
	rc = m0_bufvec_empty_alloc(&val, 1);

	memcpy(key.ov_buf[0], xkey, klen);

	rc = m0_op_kvs(M0_CLOVIS_IC_GET, &key, &val);
	if (rc)
	{
		printf("\nerror(%d): m0_op_kvs", rc);
		goto out;
	}

	rc = memcmp(key.ov_buf[0], xkey, key.ov_vec.v_count[0]);

	vlen = (size_t)val.ov_vec.v_count[0];
	memcpy(v, (char *)val.ov_buf[0], vlen);

out:
	free(xkey);
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;
}

int set_batch(char *k1, char *v1, char *ino)
{
	struct kvsns_xattr *xkey[CNT];
	size_t klen, vlen;
	int rc, i;
	struct m0_bufvec key;
	struct m0_bufvec val;

	unsigned long long int ino2;
	ino2 = atoll(ino);

	char tmpkey[256];
        struct timeval start1, end1;

        for (i = 0; i < CNT; i++)
        {
                xkey[i] = calloc(1, sizeof( struct kvsns_xattr));
                snprintf(tmpkey, 256, "%s_%d", k1, i);
                XATTR_KEY_INIT(xkey[i], ino2, tmpkey);
	}
	klen = sizeof( struct kvsns_xattr);
        vlen = strlen(v1) + 1;
	gettimeofday(&start1, NULL);

	rc = m0_bufvec_alloc(&key, CNT, klen);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_alloc", rc);
		goto out;
	}

	rc = m0_bufvec_alloc(&val, CNT, vlen);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_alloc", rc);
		goto out;
	}

	for (i = 0; i< CNT; i++)
	{
        	memcpy(key.ov_buf[i], xkey[i], klen);
		memcpy(val.ov_buf[i], v1, vlen);
	}

	rc = m0_op_kvs(M0_CLOVIS_IC_PUT, &key, &val);
	if (rc)
        {
		printf("\nerror(%d): m0_op_kvs", rc);
		goto out;
	}

  out:
       	for (i = 0; i < CNT; i ++)        	
		free(xkey[i]);
	
	m0_bufvec_free(&key);
        m0_bufvec_free(&val);
        gettimeofday(&end1, NULL);
	timer(start1, end1, "set 100 batch  keys  (nfs)");
	
	return rc;
}


int store_keyval(char *name, char *v, char *ino)
{
	struct kvsns_xattr *xkey = NULL;
	size_t klen, vlen;
	int rc;
	struct m0_bufvec key;
	struct m0_bufvec val;

	unsigned long long int ino2;
	ino2 = atoll(ino);

        xkey = calloc(1, sizeof(*xkey));
	XATTR_KEY_INIT(xkey, ino2, name);
	klen = sizeof(struct kvsns_xattr);
	vlen = strlen(v) + 1;

	rc = m0_bufvec_alloc(&key, 1, klen);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_alloc", rc);
		goto out;
	}

	rc = m0_bufvec_alloc(&val, 1, vlen);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_alloc", rc);
		goto out;
	}

        memcpy(key.ov_buf[0], xkey, klen);
	memcpy(val.ov_buf[0], v, vlen);

	rc = m0_op_kvs(M0_CLOVIS_IC_PUT, &key, &val);
	if (rc)
        {
		printf("\nerror(%d): m0_op_kvs", rc);
		goto out;
	}

  out:
         free(xkey);
	 m0_bufvec_free(&key);
         m0_bufvec_free(&val);
         return rc;
}

int set_fid()
{
	 char  tmpfid[255];
	int rc = 0;

	// Get fid from config parameter 
         memset(&ifid, 0, sizeof(struct m0_fid));
         rc = m0_fid_sscanf("<0x780000000000000b:1>", &ifid);
         if (rc != 0) {
                 fprintf(stderr, "Failed to read ifid value from conf\n");
                 goto err_exit;
         }

         rc = m0_fid_print(tmpfid, 255, &ifid);
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

int m0_search_pattern(struct kvsns_xattr *xkey)
{

	int rc;
	int *rcs;
	struct m0_bufvec keys;
	struct m0_bufvec vals;
	struct m0_clovis_op *op = NULL;

	int flags = 0;

	rcs = rcs_alloc(CNT);
	rc = m0_bufvec_alloc(&keys, CNT, KLEN);
	if (rc != 0)
	{
		printf("\n line 386");
		goto out;
	}

	rc = m0_bufvec_alloc(&vals, CNT, VALINPUT);
	if (rc != 0)
	{
		printf("\n line 386");
		goto out;
	}

	keys.ov_vec.v_count[0] = sizeof(*xkey) - sizeof(xkey->name);

	memcpy(keys.ov_buf[0], xkey, sizeof(*xkey) - sizeof(xkey->name));

	int counter = 0;

	do{
		counter ++;

		rc = m0_clovis_idx_op(&idx, M0_CLOVIS_IC_NEXT, &keys, &vals,
				      rcs, flags,  &op);
		if (rc != 0) {
			printf("\n line 375");
			goto out;
		}

		m0_clovis_op_launch(&op, 1);
		rc = m0_clovis_op_wait(op, M0_BITS(M0_CLOVIS_OS_STABLE),
				       M0_TIME_NEVER);
		if (rc != 0) {
			printf("\n line 428");
			goto out;
		}
		rc = memcmp(keys.ov_buf[0], xkey, sizeof(*xkey) - sizeof(xkey->name));
		if (rc != 0) {
			printf("\n line 386");
			goto out;
		}
		//for (i = 0; i < keys.ov_vec.v_nr; i++)
		//	printf("\nvalue %s",(char *)vals.ov_buf[i]);


		flags = M0_OIF_EXCLUDE_START_KEY;

		int j;

		for (j = 0; rc == 0 && j < CNT; j++)
			rc = rcs[j];

		m0_clovis_op_fini(op);
		memcpy(keys.ov_buf[0], keys.ov_buf[CNT-1], keys.ov_vec.v_count[CNT-1]);
	} while (rc == 0);
out:
	printf("\n counter %d\n\n", counter);

	if (rc == -ENOENT)
		printf("\nno more entries");
	else if (rc != 0)
		printf("\ninternal error");

	m0_bufvec_free(&keys);
	m0_bufvec_free(&vals);
	return rc;
}

int pattern_search(char *ino)
{
	unsigned long long int ino2;
	ino2 = atoll(ino);
	struct kvsns_xattr *xkey = NULL;
	xkey = calloc(1, sizeof(*xkey));
	XATTR_KEY_INIT(xkey, ino2, "");

	return  m0_search_pattern(xkey);

}

/* main */
int main(int argc, char **argv)
{
	char *key = malloc(sizeof(char)* 255);
	char *val = malloc(sizeof(char)* VALINPUT);

	char *ino;

	int rc = 0, i;
	struct timeval start1, end1;

	/* check input */
	if (argc != 4) {
		fprintf(stderr,"Usage:\n");
		fprintf(stderr,"%s key value\n", basename(argv[0]));
		return -1;
	}

	/* time in */
	c0appz_timein();

	/* c0rcfile
	 * overwrite .cappzrc to a .[app]rc file.
	 */
	char str[256];
	sprintf(str,".%src", basename(argv[0]));
	c0appz_setrc(str);
	c0appz_putrc();

	/* set input */
	memcpy(key, argv[1], strlen(argv[1]));
	memset(val, '*', VALINPUT);
	ino = argv[3];
	/* initialize resources */
	if (c0appz_init(0) != 0) {
		fprintf(stderr,"error! clovis initialization failed.\n");
		return -2;
	}

	c0appz_timeout(0);
	c0appz_timein();

	rc = set_fid();
	if (rc != 0)
		fprintf(stderr, "error in fid initialization");	
/*	char tmpkey[256];
	gettimeofday(&start1, NULL);
	for (i = 0; i < 100; i++)
	{
		snprintf(tmpkey, 256, "%s_%d", key, i);
		if ((rc = store_keyval(tmpkey, val, ino))!= 0)
		{
			fprintf(stderr, "%d: error in storing", rc);
			c0appz_free();
			return -3;
		}
	}
	gettimeofday(&end1, NULL);
	timer(start1, end1, "stored 100 keys one at a time (nfs)");
*/
	/* Set batch of 100 keys-Values */
	if ((rc = set_batch(key, val, ino)) != 0)
	{
		fprintf(stderr, "%d: error in storing", rc);
		c0appz_free();
		return -3;
	}

	/* Retrieving one xattr for a given inode and key*/
/*
      char tmpkey[256];
        snprintf(tmpkey, 256, "%s_%d", key, 0);
	char val2[4096];
	unsigned long long int ino2 = atoll(ino);
	if (get_keyval(tmpkey, val2, ino2) != 0)
	{
		fprintf(stderr, "error in getting value");
		c0appz_free();
		return -3;
		
	}
		printf("\n after get key %s, value %s", tmpkey, val2);
*/
	/* Listing key values */
	gettimeofday(&start1, NULL);

	pattern_search(ino);

	gettimeofday(&end1, NULL);
	timer(start1, end1, "parsed 100 keys in batch time (nfs)");
	

	printf("\nkey %s ino %s\n", key, ino);
	if ((rc = delete_batch(key, ino)) != 0)
	{
		fprintf(stderr, "%d: error in deleting batch", rc);
		c0appz_free();
		return -3;
	}
//	snprintf(tmpkey, 256, "%s_%d", key, 10);
//	memset(val, '#', VALINPUT);
	
	/* update for 1 key value */
/*	gettimeofday(&start1, NULL);

	if ((rc = store_keyval(tmpkey, val, ino))!= 0)
        {
		fprintf(stderr, "%d: error in storing", rc);
                c0appz_free();
                return -3;
	}

	gettimeofday(&end1, NULL);
	timer(start1, end1, "update 1 keys one at a time (nfs)");
*/
	/* free resources*/
	c0appz_free();

	/* time out */
	fprintf(stderr,"%4s","free");
	c0appz_timeout(0);

	/* success */
	fprintf(stderr,"%s success\n", basename(argv[0]));
	return 0;
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
