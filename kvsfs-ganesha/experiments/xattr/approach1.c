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
#include <fnmatch.h>
#define KLEN 256
#define VLEN 256

static struct m0_fid ifid;
static struct m0_ufid_generator kvsns_ufid_generator;
static struct m0_clovis_idx idx;

static int m0_op_kvs(enum m0_clovis_idx_opcode opcode, struct m0_bufvec *key, struct m0_bufvec *val)
{
	struct m0_clovis_op	 *op = NULL;
	int rcs[1];
	int rc;
	
	struct m0_clovis_idx     *index = NULL;
	index = &idx;
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
	rc = rcs[0];
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

int get_keyval(char *name, char *v, char *ino)
{
	char k[256];
	

        memset(k, 0, 256);
        snprintf(k, 256, "%s.xattr.%s", ino, name);

	size_t klen;
	size_t vlen = 256;
	
	struct m0_bufvec key;
	struct m0_bufvec val;
	int rc;

	klen = strnlen(k, 256)+1;
	
	rc = m0_bufvec_alloc(&key, 1, klen) ?:
	     m0_bufvec_empty_alloc(&val, 1);

	memcpy(key.ov_buf[0], k, klen);
	memset(v, 0, vlen);

	rc = m0_op_kvs(M0_CLOVIS_IC_GET, &key, &val);
	
	if (rc)
	{
		printf("\nerror(%d): m0_op_kvs", rc);
		goto out;
	}

	vlen = (size_t)val.ov_vec.v_count[0];
	memcpy(v, (char *)val.ov_buf[0], vlen);

out:
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;
}


int store_keyval(char *name, char *v, char *ino)
{
	char k[256]; 
	size_t klen, vlen;
	int rc;
	struct m0_bufvec key;
	struct m0_bufvec val;
 
        memset(k, 0, 256);
        snprintf(k, 256, "%s.xattr.%s", ino, name);
	klen = strnlen(k, 255)+1;
	vlen = strnlen(v, 255)+1;

	rc = m0_bufvec_alloc(&key, 1, klen) ?:m0_bufvec_alloc(&val, 1, vlen);

	 if (rc)
         {
		printf("\nerror(%d): m0_bufvec_alloc", rc);	
         	goto out;
 	 }
         memcpy(key.ov_buf[0], k, klen);
         memcpy(val.ov_buf[0], v, vlen);
 
         rc = m0_op_kvs(M0_CLOVIS_IC_PUT, &key, &val);
  out:
         m0_bufvec_free(&key);
         m0_bufvec_free(&val);
         return rc;
}

int set_fid()
{
	 char  tmpfid[255];
 	 int rc = 0;

	 /* Get fid from config parameter */
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

int m0_pattern_kvs(char *k)
{

	struct m0_bufvec keys;
	struct m0_bufvec vals;
	struct m0_clovis_op *op = NULL;
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

		rc = m0_clovis_idx_op(&idx, M0_CLOVIS_IC_NEXT, &keys, &vals,
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
			// printf("\n key %s",(char *)keys.ov_buf[i]);
			// printf("\t value %s",(char *)vals.ov_buf[i]);
			
			if (!fnmatch(k, (char *)keys.ov_buf[i], 0)) {
			/* Avoid last one and use it as first of next pass */
				if (!stop) {
					break;	
				}
				if (startp == false)
					startp = true;
			} 
			else {
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

int pattern_search(char *ino)
{
	int rc;
        char pattern[KLEN];
	char initk[KLEN];
	
	memset(pattern, 0, KLEN);
        snprintf(pattern, KLEN, "%s.xattr.*", ino);
	strncpy(initk, pattern, 256);
	initk[strnlen(pattern, KLEN)-1] = '\0';
	return  m0_pattern_kvs(initk);
}
 
void timer(struct timeval start1, struct timeval end1, char *msg)
{
	long mtime, secs, usecs;
	secs  = end1.tv_sec  - start1.tv_sec;
	usecs = end1.tv_usec - start1.tv_usec;
	mtime = ((secs) * 1000 + usecs/1000.0) + 0.5;
 	printf("Elapsed time for %s: %ld millisecs\n", msg, mtime);

}
	
/* main */
int main(int argc, char **argv)
{
	char *key = (char *)malloc(sizeof(char) * 255);
	char *val = (char *)malloc(sizeof(char) * 4096);

	char *ino;
	
	int rc = 0;
	struct timeval start1, end1;

	int i;
	/* check input */
	if (argc != 4) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s key value\n", basename(argv[0]));
		return -1;
	}

	/* time in */
	c0appz_timein();

	/* c0rcfile
	 * overwrite .cappzrc to a .[app]rc file.
	 */
	char str[256];
	sprintf(str, ".%src", basename(argv[0]));
	c0appz_setrc(str);
	c0appz_putrc();

	/* set input */
	key = argv[1];
	val = argv[2];
	ino = argv[3];
	/* initialize resources */
	if (c0appz_init(0) != 0) {
		fprintf(stderr, "error! clovis initialization failed.\n");
		return -2;
	}

	c0appz_timeout(0);
	c0appz_timein();

 	rc = set_fid();
	if (rc != 0)
		fprintf(stderr, "error in fid initialization");	
	
	 gettimeofday(&start1, NULL);
	 for ( i = 0; i < 100; i++)
	 { 
		char tmpkey[256];
		char tmpval[4096];
		snprintf(tmpkey, 256, "%s_%d", key, i);
		snprintf(tmpval, 4096, "%s_%d", val, i);	
		if ((rc = store_keyval(tmpkey, tmpval, ino))!= 0)
		{
			fprintf(stderr, "%d: error in storing", rc);
			c0appz_free();
			return -3;
		}
	}
	gettimeofday(&end1, NULL);
	timer(start1, end1, "stored 100 keys one at a time (nfs)");
	printf("\n give ino for kvs value\n");
	scanf("%s", ino);

	/* Retrieving one xattr for a given inode and key*/
        char tmpkey[256];
        snprintf(tmpkey, 256, "%s_%d", key, 0);
	char val[4096];
	if (get_keyval(tmpkey, val2, ino) != 0)
	{
		fprintf(stderr, "error in getting value");
		c0appz_free();
		return -3;
		}
//		printf("\nkey %s, value %s", tmpkey, val2);
	}

	gettimeofday(&start1, NULL);

	/* Listing all the attributes for a given  */
	pattern_search(ino);
	
	gettimeofday(&end1, NULL);
	timer(start1, end1, "parsed 100 keys one at a time (nfs)");

	/* free resources*/
	c0appz_free();

	/* time out */
	fprintf(stderr, "%4s", "free");
	c0appz_timeout(0);

	/* success */
	fprintf(stderr, "%s success\n", basename(argv[0]));
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
