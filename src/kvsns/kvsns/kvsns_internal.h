/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) CEA, 2016
 * Author: Philippe Deniel  philippe.deniel@cea.fr
 *
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/* kvsns_internal.h.c
 * KVSNS: headers for internal functions and macros
 */

#ifndef KVSNS_INTERNAL_H
#define KVSNS_INTERNAL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ini_config.h>
#include <kvsns/kvsns.h>
#include <string.h>

int kvsns_next_inode(kvsns_ino_t *ino);
int kvsns2_next_inode(void *ctx, kvsns_ino_t *ino);
int kvsns_str2parentlist(kvsns_ino_t *inolist, int *size, char *str);
int kvsns_parentlist2str(kvsns_ino_t *inolist, int size, char *str);
int kvsns_create_entry(kvsns_cred_t *cred, kvsns_ino_t *parent,
		       char *name, char *lnk, mode_t mode,
		       kvsns_ino_t *newdir, enum kvsns_type type);
int kvsns2_create_entry(void *ctx, kvsns_cred_t *cred, kvsns_ino_t *parent,
			char *name, char *lnk, mode_t mode, kvsns_ino_t *newdir,
			enum kvsns_type type);
int kvsns_get_stat(kvsns_ino_t *ino, struct stat *bufstat);
int kvsns2_get_stat(void *ctx, kvsns_ino_t *ino, struct stat *bufstat);
int kvsns_set_stat(kvsns_ino_t *ino, struct stat *bufstat);
int kvsns2_set_stat(void *ctx, kvsns_ino_t *ino, struct stat *bufstat);
int kvsns_update_stat(kvsns_ino_t *ino, int flags);
int kvsns_amend_stat(struct stat *stat, int flags);
int kvsns_delall_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino);
int kvsns_prepare_dirent_key(kvsns_ver_t ver, const kvsns_ino_t dino,
			     const uint8_t namelen, const char *name,
			     kvsns_dentry_key_t *key);

#endif
