/*
 * Filename: efs_export.h
 * Description:  Declararions of functions used to test efs_export
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

#include "efs.h"

/* endpoint dummy ops.
 *
 * @param - void.
 *
 * @return dummy endpoint operations.
 */
const struct efs_endpoint_ops* get_endpoint_dummy_ops(void);
