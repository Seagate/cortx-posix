/**
 * @file error.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  Defines error processing routines.
 */
#include <stdio.h>
#include <string.h>
#include <error.h>

static kvsns_error_t kvsns_unknown_err(int err)
{
	return KVSNS_FAIL;
}

static const char* kvsns_unknown_err_str(int err)
{
	char buf[32];
	char *cp;

	snprintf(buf, sizeof(buf), "Unknown sys error : %d", err);
	cp = strndup(buf, sizeof(buf));
	return cp != NULL ? cp : "Unknown sys error";
}

#define KVSNS_ERROR_GRN(name, _) case name: return KVSNS_ ## name;
kvsns_error_t kvsns_error(int err)
{
	switch(err) {
		KVSNS_ERRNO_MAP(KVSNS_ERROR_GRN)
	}

	return kvsns_unknown_err(err);
}
#undef KVSNS_ERROR_GRN

#define KVSNS_STRERROR_GRN(name, msg) case KVSNS_ ## name: return msg;
const char* kvsns_strerror(int err)
{
	switch(err) {
		KVSNS_ERRNO_MAP(KVSNS_STRERROR_GRN)
	}

	return kvsns_unknown_err_str(err);
}
#undef KVSNS_STRERROR_GRN
