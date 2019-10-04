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
#define MAXVAL 70000
#define VALINPUT 512 

static struct m0_fid ifid;
static struct m0_ufid_generator kvsns_ufid_generator;
static struct m0_clovis_idx idx;

static int m0_op_kvs(enum m0_clovis_idx_opcode opcode, struct m0_bufvec *key, struct m0_bufvec *val)
{
	struct m0_clovis_op *op = NULL;
	int rcs[1];
	int rc;
	
	struct m0_clovis_idx *index = NULL;
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

int parse(char *v)
{
	const char *tmp = v;

	struct json_object *obj = json_tokener_parse(tmp);

	const char *kval;
	json_object_object_foreach(obj, key, val)
	{
		kval = json_object_to_json_string(val);
	}
	return 0;
}

int json_get(char *k, char *v)
{
	size_t klen;
	size_t vlen;

	struct m0_bufvec	 key;
	struct m0_bufvec	 val;
	int rc;

	klen = strnlen(k, KLEN)+1;

	rc = m0_bufvec_alloc(&key, 1, klen);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_alloc", rc);
		goto out;
	}

	rc = m0_bufvec_empty_alloc(&val, 1);
	if (rc)
        {
		printf("\nerror(%d): m0_bufvec_empty_alloc", rc);
		goto out;
	}

	memcpy(key.ov_buf[0], k, klen);

	rc = m0_op_kvs(M0_CLOVIS_IC_GET, &key, &val);
	if (rc)
	{
		printf("\nerror(%d): m0_op_kvs while json_get", rc);
		goto out;
	}

	vlen = (size_t)val.ov_vec.v_count[0];

	memcpy(v, (char *)val.ov_buf[0], vlen);

out:
	m0_bufvec_free(&key);
	m0_bufvec_free(&val);
	return rc;

}

int in_clovis(char *k, const char *v)
{
	size_t klen, vlen;
        int rc;
	struct m0_bufvec key;
	struct m0_bufvec val;

	klen = strnlen(k, 255) + 1;
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

	memcpy(key.ov_buf[0], k, klen);
        memcpy(val.ov_buf[0], v, vlen);

        rc = m0_op_kvs(M0_CLOVIS_IC_PUT, &key, &val);
  out:
        m0_bufvec_free(&key);
        m0_bufvec_free(&val);
        return rc;
}

int json_store(char *k, char *v, char *ino)
{
	struct json_object  *obj;
	int rc = 0;
	int i;
	char tmpkey[256];

	obj = json_object_new_object();
	for (i = 0; i < 100; i++)
	{
		snprintf(tmpkey, 256, "%s_%d", k, i);
		json_object_object_add(obj, tmpkey, json_object_new_string(v));
	}

	const char *json_string = json_object_to_json_string(obj);
	//printf("json string len %d\n",strlen(json_string)); 

	rc = in_clovis(ino, json_string);
	return rc;
}

int json_update(char *k, char *v, char *ino, int no_of_keys)
{
	struct json_object *obj;
	int rc = 0, i;

	const char *tmp = malloc( sizeof(char) * MAXVAL);
	char tmpkey[256];

	json_get(ino, tmp);

	if (tmp != NULL)
		obj = json_tokener_parse(tmp);

	for (i = 0; i < no_of_keys; i++)
	{
		snprintf(tmpkey, 256, "%s_%d", k, i);
		json_object_object_add(obj, k, json_object_new_string(v));
	}

	const char *json_string = json_object_to_json_string(obj);

        rc = in_clovis(ino, json_string);

	return rc;
}

int json_delete(char *k, char *ino)
{
	struct json_object *obj;
	int rc = 0, i;
	const char *tmp = malloc( sizeof(char) * MAXVAL);
	char tmpkey[256];
	json_get(ino, tmp);
	if (tmp != NULL)
		obj = json_tokener_parse(tmp);
	else
		return -ENOENT;
	for (i = 0; i < 100; i++)
	{
		snprintf(tmpkey, 256, "%s_%d", k, i);
		json_object_object_del(obj, tmpkey);
	}
	const char *json_string = json_object_to_json_string(obj);

        rc = in_clovis(ino, json_string);

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
	char *key = malloc(sizeof(char) * 255);
	char *val = malloc(sizeof(char) * 4096);

	char *ino;
	
	int rc = 0;
	struct timeval start1, end1;

	int i;
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
	sprintf(str, ".%src", basename(argv[0]));
	c0appz_setrc(str);
	c0appz_putrc();

	/* set input */
	memcpy(key, argv[1], strlen(argv[1]));
	memset(val, '*', VALINPUT);
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

	rc = json_store(key, val, ino);
	if (rc != 0)
	{
		fprintf(stderr, "error in storing json value");
		c0appz_free();
		return -3;
	}

	gettimeofday(&end1, NULL);
	timer(start1, end1, "stored 100 keys in json");

	printf("Stored successfully json object in clovis");
	
	int no_of_keys;
	printf("\n give no of keys to update in json\n");
	scanf("%d", &no_of_keys);

	memset(val, '#', VALINPUT);
	gettimeofday(&start1, NULL);

	rc = json_update(key, val, ino, no_of_keys);	
	if (rc != 0)
	{
		fprintf(stderr, "error in updating json value");
		c0appz_free();
		return -3;
	}

	gettimeofday(&end1, NULL);
	timer(start1, end1, "updated key in json");

	char val3[MAXVAL];
	printf("\n give ino for json value\n");
	scanf("%s", ino);

	gettimeofday(&start1, NULL);

	rc = json_get(ino, val3);
	if (rc != 0)
	{
		fprintf(stderr, "error in getting json value");
		c0appz_free();
		return -3;
	}

	parse(val3);
	//printf("json object retrieved %s", val3);

	gettimeofday(&end1, NULL);
	timer(start1, end1, "parsed 100 keys in json");
	ino = NULL;
	ino = malloc(256);
	printf("\n give ino for json value\n");
	scanf("%s", ino);


	gettimeofday(&start1, NULL);

	rc = json_delete(key, ino);
	if (rc != 0)
	{
		fprintf(stderr, "error in deleting json value");
		c0appz_free();
		return -3;
	}

	gettimeofday(&end1, NULL);
	timer(start1, end1, "deleted 100 keys and values in json");


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
