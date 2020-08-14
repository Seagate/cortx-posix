/*
 * Filename:         dsal.h
 * Description:      Provides DSAL specific implementation.
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

#ifndef _DSAL_H
#define _DSAL_H

#include "dstore.h" /* header for dstore */

/* Initialize dsal.
 * 
 * @param cfg[] - collection item, int flags.
 * 
 * @return 0 if successful, a negative "-errno" value in case of failure.
 */
int dsal_init(struct collection_item *cfg, int flags);

/* Finalize dsal.
 *  
 *  @param void.
 *  
 *  @return 0 if successful, a negative "-errno" value in case of failure.
 */

int dsal_fini(void);

#endif
