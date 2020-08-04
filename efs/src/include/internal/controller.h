/**
 * Filename: controller.h
 * Description: Controller headers:
 * 		Defines the controller types and it's api's.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation, distribution
 * or disclosure of this code, for any reason, not expressly authorized is
 * prohibited. All other rights are expressly reserved by Seagate Technology, LLC.
 *
 * Author: Yogesh Lahane <yogesh.lahane@seagate.com>
 *
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
	XX(ENDPOINT,	endpoint)
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

#endif
