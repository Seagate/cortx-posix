/*
 * Filename: nsal.h
 * Description: Data types and declarations for NSAL.
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

#ifndef _NSAL_H_
#define _NSAL_H_

typedef obj_id_t node_id_t; /* Act as node identifier in kvtree */

typedef enum kvtree_key_type {
	/* KVTree key types associated with particular version of kvtree. */
	KVTREE_KEY_TYPE_CHILD = 20,
	KVTREE_KEY_TYPE_BASIC_ATTR,
	KVTREE_KEY_TYPE_SYSTEM_ATTR,
	/* TODO: This enum entry should be removed because xattr is
	 * independent of kvtree. Right now we need it here to keep
	 * backward compatibility with kvtree key types and avoid conflict
	 * with xattr keys as both are stored in same index.
	 * When seperate xattr key type is added, this entry can be safely
	 * removed.
	 **/
	KVTREE_KEY_TYPE_XATTR,
	KVTREE_KEY_TYPE_INVALID,
	KVTREE_KEY_TYPES = KVTREE_KEY_TYPE_INVALID,

} kvtree_key_type_t;


/******************************************************************************/
/* Methods */

/** Initialize nsal.
 *
 *  @param "struct collection_item *cfg_items".
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int nsal_module_init(struct collection_item *cfg_items);

/** finalize nsal.
 *
 *  @param void.
 *
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */
int nsal_module_fini(void);

#endif /* _NSAL_H_ */
