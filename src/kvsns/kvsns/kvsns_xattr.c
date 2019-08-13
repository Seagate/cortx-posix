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

/* kvsns_xattr.c
 * KVSNS: implementation of xattr feature
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>
#include "kvsns_internal.h"

int kvsns_setxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t size, int flags)
{
	/* Set or create (XATTR_CREATE) xattr by name (<inode>.xattr.name) */
	return 0;
}

int kvsns_getxattr(kvsns_cred_t *cred, kvsns_ino_t *ino,
		   char *name, char *value, size_t *size)
{
	/* Get <inode>.xattr.name */
	return -ENOENT;
}

int kvsns_listxattr(kvsns_cred_t *cred, kvsns_ino_t *ino, int offset,
		  kvsns_xattr_t *list, int *size)
{
	/* Get list of all (<inode>.xattr.*) */
	return -ENOENT;
}

int kvsns_removexattr(kvsns_cred_t *cred, kvsns_ino_t *ino, char *name)
{
	/* TODO: Remove xattr by name (<inode>.xattr.name) */
	return 0;
}

int kvsns_remove_all_xattr(kvsns_cred_t *cred, kvsns_ino_t *ino)
{
	/* TODO: get all <inode>.xattr.* entries and remove them */
	return 0;
}
