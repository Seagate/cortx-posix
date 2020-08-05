/*
 * Filename:         redis_kvstore.h
 * Description:      Provides EOS specific implementation of
 *                  KVStore module of NSAL.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This REDIS specific implementation for kvstore.
*/

#include "kvstore.h"
#include <common/helpers.h>
#include <errno.h>

extern struct kvstore_ops redis_kvs_ops;
