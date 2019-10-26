/**
 * @file eosfs.h
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  Headers for op's.
 */

#ifndef _TEST_KVSNS_COMMON_H
#define _TEST_KVSNS_COMMON_H

#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

/**
 * eosfs.c function prototype declarations.
 */
void eos_fs_init(void);
void eos_fs_fini(void);
void eos_fs_open(uint64_t fs_id, kvsns_fs_ctx_t *fs_ctx);

#endif
