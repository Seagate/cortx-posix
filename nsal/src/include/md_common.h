/*
 * Filename: md_common.h
 * Description: Metadata - Data types and declarations for common metadata api.
 *
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com. 
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

#define MD_XATTR_SIZE_MAX 4096

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
