/**
 * Filename:         fsal_global_tables.c
 * Description:      FSAL CORTXFS's global table implementation related
 *                   data structures and associated APIs
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 * Author: Pratyush Kumar Khan <pratyush.k.khan@seagate.com>
 */

#include "config.h"

#include <assert.h>
#include <libgen.h>		/* used for 'dirname' */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <mntent.h>
#include "gsh_list.h"
#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "FSAL/fsal_config.h"
#include "FSAL/fsal_commonlib.h"
#include "kvsfs_methods.h"
#include "nfs_exports.h"
#include "nfs_creds.h"
#include "pnfs_utils.h"
#include <stdbool.h>
#include <arpa/inet.h>


#define CORTXFS_HASH_TABLE_SIZE 64

struct cortxfs_hash_elem {
	uint16_t helm_ref; // TODO: implement it
	void *helm;
	size_t helm_size;
	LIST_ENTRY(cortxfs_hash_elem) helm_link;
};

struct cortxfs_hash_bucket {
	pthread_mutex_t hbt_mutex;
	LIST_HEAD(hbt_list, cortxfs_hash_elem) hbt_chain;
};

struct cortxfs_hash_table {
	struct cortxfs_hash_bucket ht_buckets[CORTXFS_HASH_TABLE_SIZE];
};

struct cortxfs_gtbl {
	enum cortxfs_gtbl_types gt_type;
	struct cortxfs_hash_table gt_ht;
	/**
	 * TODO: add private ctx and recall handler for async free
	 * This will be needed when a HT elem (e.g. a layout) with non-zero
	 * references are being removed. This removal should trigger the
	 * associated table's recall handler, that handler will inform the
	 * client that the active layout is being removed.
	 */
};

static struct cortxfs_gtbl g_tbls[CORTXFS_GTBL_STATE_END];

static void gtbl_htbl_ini(struct cortxfs_hash_table *ht)
{
	int rc;
	int idx = 0;

	memset(ht, 0, sizeof(*ht));

	while (idx < CORTXFS_HASH_TABLE_SIZE) {
		rc = pthread_mutex_init(&ht->ht_buckets[idx].hbt_mutex, NULL);
		dassert(rc == 0);
		idx++;
	}
}

static void gtbl_htbl_fini(struct cortxfs_hash_table *ht)
{
	int rc;
	int idx = 0;
	struct cortxfs_hash_bucket *hbt;
	struct cortxfs_hash_elem *helem;

	memset(ht, 0, sizeof(*ht));

	while (idx < CORTXFS_HASH_TABLE_SIZE) {
		hbt = &ht->ht_buckets[idx];

		while (!LIST_EMPTY(&hbt->hbt_chain)) {
			helem = LIST_FIRST(&hbt->hbt_chain);
			LIST_REMOVE(helem, helm_link);
			// TODO: if helm_ref != 0, use recall/upcall/teardown
			gsh_free(helem->helm);
			gsh_free(helem);
		}

		rc = pthread_mutex_destroy(&ht->ht_buckets[idx].hbt_mutex);
		dassert(rc == 0);
		idx++;
	}
}

void gtbl_ini(void)
{
	gtbl_htbl_ini(&g_tbls[CORTXFS_GTBL_STATE_LAYOUTS].gt_ht);
	// TODO: add future tables here, one global for each type
}

void gtbl_fini(void)
{
	gtbl_htbl_fini(&g_tbls[CORTXFS_GTBL_STATE_LAYOUTS].gt_ht);
	// TODO: add future tables here, one global for each type
}

int gtbl_add_elem(enum cortxfs_gtbl_types type, void *elem, size_t elem_size,
		   uint64_t key)
{
	int rc, rc1 = 0;
	struct cortxfs_hash_bucket *hbt;
	struct cortxfs_hash_elem *helem;
	uint16_t hkey = key % CORTXFS_HASH_TABLE_SIZE;

	dassert(type >= CORTXFS_GTBL_STATE_END);
	dassert(elem != NULL);

	hbt = &g_tbls[type].gt_ht.ht_buckets[hkey];

	rc = pthread_mutex_lock(&hbt->hbt_mutex);
	dassert(rc == 0);

	LIST_FOREACH(helem, &hbt->hbt_chain, helm_link) {
		if ((helem->helm_size == elem_size) &&
		    (memcmp(helem->helm, elem, elem_size) == 0)) {
			// already present, do nothing
			rc1 = EEXIST;
			goto out;
		}
	}

	helem = gsh_calloc(1, sizeof(*helem));
	dassert(helem != NULL);
	helem->helm = elem;
	LIST_INSERT_HEAD(&hbt->hbt_chain, helem, helm_link);

out:
	rc = pthread_mutex_unlock(&hbt->hbt_mutex);
	dassert(rc == 0);

	return rc1;
}

void * gtbl_find_elem(enum cortxfs_gtbl_types type, const void *elem,
			     size_t elem_size, uint64_t key)
{
	int rc;
	void *elem_ret = NULL;
	struct cortxfs_hash_bucket *hbt;
	struct cortxfs_hash_elem *helem;
	uint16_t hkey = key % CORTXFS_HASH_TABLE_SIZE;

	dassert(type >= CORTXFS_GTBL_STATE_END);
	dassert(elem != NULL);

	hbt = &g_tbls[type].gt_ht.ht_buckets[hkey];

	rc = pthread_mutex_lock(&hbt->hbt_mutex);
	dassert(rc == 0);

	LIST_FOREACH(helem, &hbt->hbt_chain, helm_link) {
		if ((helem->helm_size == elem_size) &&
		    (memcmp(helem->helm, elem, elem_size) == 0)) {
			elem_ret = helem->helm;
			break;
		}
	}

	rc = pthread_mutex_unlock(&hbt->hbt_mutex);
	dassert(rc == 0);

	return elem_ret;
}

void * gtbl_remove_elem(enum cortxfs_gtbl_types type, const void *elem,
			   size_t elem_size, uint64_t key)
{
	int rc;
	void *elem_rmv = NULL;
	struct cortxfs_hash_bucket *hbt;
	struct cortxfs_hash_elem *helem;
	uint16_t hkey = key % CORTXFS_HASH_TABLE_SIZE;

	dassert(elem != NULL);

	hbt = &g_tbls[type].gt_ht.ht_buckets[hkey];

	rc = pthread_mutex_lock(&hbt->hbt_mutex);
	dassert(rc == 0);

	LIST_FOREACH(helem, &hbt->hbt_chain, helm_link) {
		if ((helem->helm_size == elem_size) &&
		    (memcmp(helem->helm, elem, elem_size) == 0)) {
			elem_rmv = helem->helm;
			break;
		}
	}

	if (elem_rmv) {
		LIST_REMOVE(helem, helm_link);
		gsh_free(helem);
	}

	rc = pthread_mutex_unlock(&hbt->hbt_mutex);
	dassert(rc == 0);

	return elem_rmv;
}
