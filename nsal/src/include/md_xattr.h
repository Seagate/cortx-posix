/*
 * Filename: md_xattr.h
 * Description: xattr - Data types and declarations for  NSAL xattr api.
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

#ifndef MD_XATTR__H
#define MD_XATTR__H

#include <md_common.h>

/* Forward declarations. */
struct kvs_idx;

/* Common Metadata Xattr apis */
/**
 * Sets an xattr to an fid entry
 *
 * @note: xattr's name are zero-terminated strings.
 *
 * @param[in] fctx - File system ctx
 * @param[in] fid - 128 bit File (object) identifier
 * @param[in] name - xattr's name
 * @param[in] value - buffer with xattr's value
 * @param[in] size - size of passed buffer
 *
 * @return 0 if successful, a negative "-errno" value in case of failure
 */

int md_xattr_set(struct kvs_idx *idx, const obj_id_t *oid,
		 const char *name, const void *value, size_t size);

/**
 * Gets an xattr for an fid
 *
 * @note: xattr's name is a zero-terminated string.
 *
 * @param[in] fctx - File system ctx
 * @param[in] fid - 128 bit File (object) identifier.
 * @param[in] name - xattr's name
 * @param[out] value - Allocated buffer in which xattr's value has been copied.
 *		       The caller should free this by calling md_free()
 * @param[out] size - Size of value buffer
 *
 * @return zero if successful, negative "-errno" value in case of
 *         failure
 */
int md_xattr_get(struct kvs_idx *idx, const obj_id_t *oid,
		 const char *name, void **value, size_t *size);

/**
 * Checks if an xattr exists for a fid.
 *
 * @note: xattr's name is a zero-terminated string.
 *
 * @param[in] fctx - File system ctx
 * @param[in] fid - 128 bit File (object) identifier.
 * @param[in] name - xattr's name
 * @param[out] result - true if xattr with passed name exists. False otherwise.
 *
 * @return zero if successful, negative "-errno" value in case of
 *         failure.
 */
int md_xattr_exists(struct kvs_idx *idx, const obj_id_t *oid,
		    const char *name, bool *result);

/**
 * Deletes  an xattr for an fid
 *
 * @note: xattr's name is a zero-terminated string.
 *
 * @param[in] fctx - File system ctx
 * @param[in] fid - 128 bit File (object) identifier.
 * @param[in] name - xattr's name
 *
 * @return zero if xattr has been deleted successfully,  negative "-errno" value
 *	    in case of failure
 */
int md_xattr_delete(struct kvs_idx *idx, const obj_id_t *oid,
		    const char *name);

/**
 * Fetches xattr names for a fid.
 *
 * @note: xattr's name is a zero-terminated string.
 *
 * @param[in] fctx - File system ctx
 * @param[in] fid - 128 bit File (object) identifier.
 * @param[out] buf - The list which has the set of (null-terminated)
 *		     names, one after the other.
 * @param[out] count - Count of extended attribute names in the buf
 * @param[in] - Size of buf. If the passed size is zero, the size is set to
 *		current size of the the fetched attribute name list
 *	 [out] - Size of the fetched attribute name list.
 *
 *
 * @return zero for success, negative "-errno" value in case of failure
 */
int md_xattr_list(struct kvs_idx *idx, const obj_id_t *oid, void *buf,
		  size_t *count, size_t *size);

/** Frees a passed md pointer. */
void md_xattr_free(void *ptr);

#endif
