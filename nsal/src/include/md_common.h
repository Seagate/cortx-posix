/*
 * Filename: md_common.h
 * Description: Metadata - Data types and declarations for common metadata api.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Subhash Arya <subhash.arya@seagate.com>
 */

#ifndef _MD_COMMON_H
#define _MD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <inttypes.h>
#include <kvstore.h>
#include <object.h>

#define MD_DASSERT(cond) assert(cond)

#define MD_RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc < 0)        \
		return __rc; })

#define MD_RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	if (__rc < 0)        \
		goto __label; })

/** Version of a common metadata representation. */
typedef enum md_version {
	MD_VERSION_0 = 0,
	MD_VERSION_INVALID,
} md_version_t;

/* Common md key types associated with particular version of md. */
typedef enum md_key_type {
	MD_KEY_TYPE_XATTR = 1,
	MD_KEY_TYPE_INVALID,
	MD_KEY_TYPES = MD_KEY_TYPE_INVALID,
} md_key_type_t;

/* key metadata which should be included into each md key (2 bytes) */
struct md_key_md {
	uint8_t k_type;
	uint8_t k_version;
} __attribute__((packed));
typedef struct md_key_md md_key_md_t;

/** A string object which has a C string (up to 255 characters)
 * and its length.
 */
struct md_str256 {
	/** The length of the C string. */
	uint8_t len;
	/** A buffer which contains a null-terminated C string. */
	char    str[NAME_MAX + 1];
} __attribute__((packed));

/** Name of an object in a file tree. */
typedef struct md_str256 md_str_t;

/** Convert a C-string into a md name.
 * @param[in] name - a C-string to be converted.
 * @param[out] name - a md name.
 * @return Len of the string or -EINVAL.
 */
int md_name_from_cstr(const char *name, md_str_t *lstr);

/** Frees a passed md pointer. */
void md_free(void *ptr);

#endif
