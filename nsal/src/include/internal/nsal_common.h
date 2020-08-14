/*
 * Filename: nsal_common.h
 * Description: tenant - nsal common data structures
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

#ifndef _NSAL_COMMON_H_
#define _NSAL_COMMON_H_

/* Key prefix */
struct key_prefix {
	uint8_t k_type;
	uint8_t k_version;
} __attribute__((packed));

typedef enum nsal_version {
	NS_VERSION_0 = 0,
	NS_VERSION_INVALID,
} nsal_version_t;

/* namespace key types associated with particular version of ns. */
typedef enum ns_key_type {
	NS_KEY_TYPE_NS_INFO = 1, /* Key for storing namespace information. */
	NS_KEY_TYPE_NS_ID_NEXT,  /* Key for storing id of namespace. */
	NS_KEY_TYPE_TENANT_INFO,  /* Key for storing tenant information. */
	NS_KEY_TYPE_TENANT_ID_NEXT,  /* Key for storing id of tenant */
	NS_KEY_TYPE_INVALID,
} ns_key_type_t;

/* namespace key */
struct ns_key {
	struct key_prefix ns_prefix;
	uint16_t ns_id;
} __attribute((packed));

#define NS_KEY_INIT(_key, _ns_id, _ktype)		\
{							\
	_key->ns_id = _ns_id,				\
	_key->ns_prefix.k_type = _ktype,		\
	_key->ns_prefix.k_version = NS_VERSION_0;	\
}

#define NS_KEY_PREFIX_INIT(_key, _type)		\
{						\
	_key->k_type = _type,			\
	_key->k_version = NS_VERSION_0;		\
}

/* tenant key */
struct tenant_key {
	struct key_prefix tenant_prefix;
	uint16_t tenant_id;
} __attribute((packed));

#define TENANT_KEY_INIT(_key, _tenant_id, _ktype)		\
{								\
	_key->tenant_id = _tenant_id,				\
	_key->tenant_prefix.k_type = _ktype,			\
	_key->tenant_prefix.k_version = NS_VERSION_0;		\
}

#define TENANT_KEY_PREFIX_INIT(_key, _type)	\
{						\
	_key->k_type = _type,			\
	_key->k_version = NS_VERSION_0;		\
}
#endif /* _NSAL_COMMON_H_ */
