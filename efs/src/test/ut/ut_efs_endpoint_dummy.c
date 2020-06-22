/*
  Filename: ut_efs_endpoint_ops_dummy.c
 * Description: This file implements intialization of dummy endpoint operation.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2020, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Jatinder Kumar <jatinder.kumar@seagate.com>
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
