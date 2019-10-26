/**
 * @file cp.h
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  Headers for cp.c
 */

#ifndef _TEST_KVSNS_CP_H
#define _TEST_KVSNS_CP_H

#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>


#define IOLEN 4096
#define BUFFSIZE 40960

/**
 * @brief copy from kvsns to posix 
 * 
 * @param[in] fs_ctx		Filesystem context.
 * @param[in] user_cred		User credentials.
 * @param[in] kfd		kvsns open file handle.
 * @Param[in] offset		Copy offset in kfd
 * @Param[in] size		Bytes size
 * @parma[in] fd		Destination fd 
 *
 * @return 			returns success = 0, failure = -1.
 */
int cp_from_kvsns(void *fs_ctx, kvsns_cred_t *usr_cred,
		kvsns_file_open_t *kfd, off_t offset, size_t size, int fd);

/**
 * @brief copy to kvsns from posix 
 * 
 * @param[in] fs_ctx		Filesystem context.
 * @param[in] user_cred		User credentials.
 * @param[in] kfd		kvsns open file handle.
 * @Param[in] offset		Copy offset in kfd
 * @Param[in] size		Bytes size
 * @parma[in] fd		Source fd 
 *
 * @return 			returns success = 0, failure = -1.
 */
int cp_to_kvsns(void *fs_ctx, kvsns_cred_t *cred,
		kvsns_file_open_t *kfd, off_t offset, size_t size, int fd);

/**
 * @brief copy to kvsns from kvsns 
 * 
 * @param[in] fs_ctx		Filesystem context.
 * @param[in] user_cred		User credentials.
 * @param[in] src_kfd		Source kvsns open file handle.
 * @param[in] dest_kfd		Destination kvsns open file handle.
 *
 * @return 			returns success = 0, failure = -1.
 */
int cp_to_and_from_kvsns(void *fs_ctx, kvsns_cred_t *usr_cred,
		kvsns_file_open_t *src_kfd, kvsns_file_open_t *dest_kfd);

#endif
