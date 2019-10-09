/*
 * log.h
 * Header file for KVSNS logging interfaces
 */

#ifndef KVSNS_LOG_H
#define KVSNS_LOG_H

#include <stdio.h>

/* @todo : Improve this simplistic implementation of logging. */
#define KVSNS_LOG(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)

/* @todo: Add more logging levels. */
#define log_err KVSNS_LOG
#define log_warn KVSNS_LOG
#define log_info KVSNS_LOG
#define log_debug KVSNS_LOG
#define log_trace KVSNS_LOG

/* Set source of an error.
 * Allows to find the place where an errno return code has been set.
 * This macro can be used in assignments and return statements:
 *
 *	if (!lookup(fd)) {
 *		return KVSNS_SET_ERROR(-ESTALE);
 *	}
 *
 *	if (strlen(str) > 255) {
 *		rc = KVSNS_SET_ERROR(-EINVAL);
 *		goto out;
 *	}
 *
 */
#define KVSNS_SET_ERROR(__err) (log_trace("set_error: %d (%d)", __err, -__err), __err)
#endif
