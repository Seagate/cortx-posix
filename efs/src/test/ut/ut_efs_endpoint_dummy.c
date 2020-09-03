/*
  Filename: ut_efs_endpoint_ops_dummy.c
 * Description: This file implements intialization of dummy endpoint operation.
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

#include "ut_efs_endpoint_dummy.h"

static int dummy_config_init(void)
{
	printf("%s: ()\n", __func__);
	return 0;
}

static int dummy_config_fini(void)
{
	printf("%s: ()\n", __func__);
	return 0;
}

static int dummy_add_export(const char *ep_name, uint16_t ep_id,
			    const char *ep_info)
{
	printf("%s: (name=%s, id=%u, ep_info=%s)\n", __func__, ep_name, ep_id,
	      ep_info);
	return 0;
}

static int dummy_remove_export(uint16_t ep_id)
{
	printf("%s: (id=%u)\n", __func__, ep_id);
	return 0;
}

/* dummuy endpoint operations */
static const struct efs_endpoint_ops g_dummy_ep_ops = {
	.init = dummy_config_init,
	.fini = dummy_config_fini,
	.create = dummy_add_export,
	.delete = dummy_remove_export,
};

const struct efs_endpoint_ops* get_endpoint_dummy_ops(void)
{
	return &g_dummy_ep_ops;
}
