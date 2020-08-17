/*
 * Filename: controller.h
 * Description: Controller headers:
 * 		Defines the controller types and it's api's.
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

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#define COUNT(...) +1

/**
 * ######################################################################
 * #		Control-Server: CONTROLLER Types.			#
 * ######################################################################
 */
#define CONTROLLER_MAP(XX)		\
	XX(FS,		fs)		\
	XX(ENDPOINT,	endpoint)	\
	XX(AUTH,	auth)
/**
 * Not supporeted yet.
 * Add to the CONTROLER_MAP as and when supported.
 * XX(FS,	fs)		\
 * XX(ENDPOINT,	endpoint)		\
 * XX(FAULT,	fault)		\
 */

#define CONTROLLER_COUNT	(0+CONTROLLER_MAP(COUNT))

/**
 * Controller Types.
 */
enum controller_type {

#define XX(uc, lc)		\
	CONTROLLER_ ## uc ## _ID,
	CONTROLLER_MAP(XX)
#undef XX

};

/**
 * Controller functions.
 */
#define XX(uc, lc)								\
	int ctl_ ## lc ## _init(struct server *server,				\
				struct controller **controller);		\
	void ctl_ ## lc ## _fini(struct controller *controller);
	CONTROLLER_MAP(XX)
#undef XX


/**
 * ######################################################################
 * #		Control-Server: CONTROLLER APIs.			#
 * ######################################################################
 */
#define FS_API_MAP(XX)					\
	XX(CREATE,	create,		PUT)		\
	XX(DELETE,	delete,		DELETE)		\
	XX(LIST,	list,		GET)

enum fs_api_id {
#define XX(uc, lc, _)	FS_ ## uc ## _ID,
	FS_API_MAP(XX)
#undef XX
};

#define FS_API_COUNT   (0+FS_API_MAP(COUNT))

#define ENDPOINT_API_MAP(XX)				\
	XX(CREATE,	create,		PUT)		\
	XX(DELETE,	delete,		DELETE)

enum endpoint_api_id {
#define XX(uc, lc, _)	ENDPOINT_ ## uc ## _ID,
	ENDPOINT_API_MAP(XX)
#undef XX
};

#define ENDPOINT_API_COUNT   (0+ENDPOINT_API_MAP(COUNT))

#define AUTH_API_MAP(XX)				\
	XX(SETUP,	setup,		PUT)		\
	XX(REMOVE,	remove,		DELETE)

enum auth_api_id {
#define XX(uc, lc, _)	AUTH_ ## uc ## _ID,
	AUTH_API_MAP(XX)
#undef XX
};

#define AUTH_API_COUNT   (0+AUTH_API_MAP(COUNT))

#endif
