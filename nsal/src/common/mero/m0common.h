/*
 * Filename:         m0common.h
 * Description:      Contains implementation mero helpers

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 @todo Move this code to utils/src/eos/helpers.c file, after
 iterators are generalized. (EOS-4351)
*/

#ifndef _M0COMMON_H
#define _M0COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <fnmatch.h>
#include <ini_config.h>

#include <kvstore.h>
#include "clovis/clovis.h"
#include "clovis/clovis_internal.h"
#include "clovis/clovis_idx.h"

#ifdef DEBUG
#define M0_DASSERT(cond) assert(cond)
#else
#define M0_DASSERT(cond)
#endif

/******************************************************************************/
/* Key Iterator */

/* TODO:PERF:
 *	Performance of key iterators can be improved by:
 *	1. Usage of prefetch.
 *	2. Async Clovis calls.
 *	3. Piggyback data for records.
 *	The features can be implemented without significant changes in
 *	the caller code and mostly isolated in the m0common module.
 *
 *	1. The key prefetch feature requires an additional argument to specify
 *	the amount of records to retrieve in a NEXT clovis call.
 *	Then, key_iter_next walks over the prefetched records and issues
 *	a NEXT call after the last portion of records was processed by the user.
 *
 *	2. The async clovis calls feature can be used to speed up the case
 *	where the time of records processing by the caller is comparable with
 *	the time needed to receive next bunch of records from Clovis.
 *	In this case a initial next call synchronously gets a bunch of records,
 *	and then immediately issues an asynchronous NEXT call.
 *	The consequent next call waits for the issued records,
 *	and again issues a NEXT call to clovis. In conjunction with the prefetch
 *	feature, it can help to speed up readdir() by issuing NEXT (dentry)
 *	and GET (inode attrs) concurrently.
 *
 *	3. Since readdir requires a combination of NEXT + GET calls,
 *	the iterator can issue a GET call to get the inode attirbutes of
 *	prefetched dentries along with the next portion of NEXT dentries.
 *	So that, we can get a chunk of dentries and the attributes of the
 *	previous chunck witin a single clovis call.
 *	However, this feature make sense only for the recent version of
 *	nfs-ganesha where a FSAL is resposible for filling in attrlist
 *	(the current version calls fsal_getattr() instead of it).
 */

/** Find the first record following by the prefix and set iter to it.
 * @param iter Iterator object to initialized with the starting record.
 * @param prefix Key prefix to be found.
 * @param prefix_len Length of the prefix.
 * @return True if found, otherwise False. @see kvstore_iter::inner_rc for return
 * code.
 */
bool m0_key_iter_find(struct kvstore_iter *iter, const void* prefix,
		      size_t prefix_len);

/** Cleanup key iterator object */
void m0_key_iter_fini(struct kvstore_iter *iter);

/* Find the next record and set iter to it. */
bool m0_key_iter_next(struct kvstore_iter *iter);

size_t m0_key_iter_get_key(struct kvstore_iter *iter, void **buf);
size_t m0_key_iter_get_value(struct kvstore_iter *iter, void **buf);

#endif
/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
