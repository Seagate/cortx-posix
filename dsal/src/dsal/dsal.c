/*
 * Filename:         dsal.c
 * Description:      Contains DSAL specific implementation.
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

#include "dsal.h"  /* header for dsal_init/dsal_fini */
#include <debug.h> /* dassert */

static int dsal_initialized;

int dsal_init(struct collection_item *cfg, int flags)
{
	dassert(dsal_initialized == 0);
	dsal_initialized++;
	return dstore_init(cfg, flags);
}

int dsal_fini(void)
{
	struct dstore *dstor = dstore_get();
        dassert(dstor != NULL);
	return dstore_fini(dstor);
}

