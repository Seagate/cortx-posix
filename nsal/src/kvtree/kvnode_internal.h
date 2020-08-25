/*
 * Filename:         kvnode_internal.h
 * Description:      kvnode - Data types and declarations for kvnode -
 *                   component of kvtree.
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

#ifndef _KVNODE_INTERNAL_H
#define _KVNODE_INTERNAL_H

/* On-disk structure for storing node basic attributes.
 * Uses cases: CORTXFS file stats */
struct kvnode_basic_attr {
	uint16_t size;          /* size of node basic attributes buffer */
	char attr[0];           /* node basic attributes buffer */
};

#endif /* _KVNODE_INTERNAL_H */
