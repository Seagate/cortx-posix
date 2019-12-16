/**
 * Filename: ifault.h
 * Description: Headers for fault framework.
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

#ifndef _IFAULT_H
#define _IFAULT_H

#include <stdint.h>
#include <pthread.h>		/* mutexes */

#ifdef PUBLIC
	#undef PUBLIC
#endif
#ifdef PRIVATE
	#undef PRIVATE
#endif

#define PUBLIC	__attribute__ ((visibility ("default")))
#define PRIVATE	 __attribute__ ((visibility ("hidden")))

#define FAULT_INJECTION ON
/**
 * Represents the fault point set.
 * It provides interfaces to update the fault points and also check if a
 * Particular fault point is active or not.
 */

/* Fault Point Types */
typedef enum ifault_fp_type {
	/*< Triggers only on first hit, then becomes disabled automatically */
	FP_TYPE_ONCE,
	/*< Always triggers when enabled */
	FP_TYPE_ALWAYS,
	/**
	 * Doesn't trigger first N times, then triggers next M times, then
	 * repeats this cycle
	 */
	FP_TYPE_REPEAT,
	FP_TYPE_SKIP,
	/*< Sleeps for specified amount of time */
	FP_TYPE_DELAY,
	 /*< Triggers with a given probability */
	FP_TYPE_RANDOM,
	/*< User defined call back fuction */
	FP_TYPE_USER_FUNC,
	/*< Number of fault points */
	FP_TYPE_NR,

} ifault_fp_type_t;

#define FP_SPEC(fp_type, ...)							\
		(fp_type), (va_argc) __VA_OPT__(,) __VA_ARGS__

#define FP_ONCE(h_point)							\
		FP_SPEC(FP_TYPE_ONCE, 1L, (h_point))

#define FP_ALWAYS()								\
		FP_SPEC(FP_TYPE_ALWAYS, 1L, (1L))

#define FP_REPEAT(h_start, h_end)						\
		FP_SPEC(FP_TYPE_REPEAT, 2L, (h_start), (h_end))

#define FP_SKIP(h_start, h_end)							\
		FP_SPEC(FP_TYPE_SKIP, 2L, (h_start), (h_end))

#define FP_DELAY(f_delay)							\
		FP_SPEC(FP_TYPE_DELAY, 1L, (f_delay))

#define FP_RANDOM(prob)								\
		FP_SPEC(FP_TYPE_RANDOM,	1L, (f_prob))

#define FP_USER_FUNC(f_func_cb, f_func_cb_data)					\
		FP_SPEC(FP_TYPE_USER_FUNC, 2L,(f_func_cb), (f_func_cb_data))


typedef struct ifault_fp_state ifault_fp_state_t;

/**
 * Holds information about "fault point" (FP).
 */
typedef struct ifault_fp {
	/*< Subsystem/module name */
	const char		*fp_module;
	/*< File name */
	const char		*fp_file;
	/*< Line number */
	uint32_t		fp_line_num;
	/*< Function name */
	const char		*fp_func;
	/**
	 * Tag - one or several words, separated by underscores, aimed to
	 * describe a purpose of the fault point and uniquely identify this FP
	 * within a current function
	 */
	const char		*fp_tag;
	/*< Fault point state */
	ifault_fp_state_t	*fp_state;

} ifault_fp_t;

/* Fault Point ID */
typedef struct ifault_fp_id {
	/*< Fault point function name where fault is defined */
	const char *fpi_func;
	/*< Unique Tag that identifies fault point in the function */
	const char *fpi_tag;

} ifault_fp_id_t;

/**
 * Fault Framework Interfaces
 * Note: These interfaces are not thread safe
 */
PUBLIC void ifault_init(void);
PUBLIC void ifault_deinit(void);

PUBLIC int ifault_enabled(ifault_fp_t *fp);
PUBLIC void ifault_enable(ifault_fp_id_t *fp_id);
PUBLIC void ifault_disable(ifault_fp_id_t *fp_id);
PUBLIC void ifault_register(ifault_fp_t *fp, ifault_fp_type_t fp_type,
				uint32_t spec_argc, ...);
PUBLIC void ifault_print_state(ifault_fp_id_t *fp_id);

#ifdef FAULT_INJECTION
/**
 * Defines a fault point and checks if it's enabled.
 *
 * FP registration occurs only once, during first time when this macro is
 * "executed". ifault_register() is used to register FP in a global dynamic list,
 * which may introduce some delay if this list already contains large amount of
 * registered fault points.
 *
 * A typical use case for this macro is:
 *
 * @code
 *
 * void *kvsal_alloc(size_t n)
 * {
 *	...
 *	if (FP_ENABLED("pretend_failure", FP_TYPE_ALWAYS, FP_ALWAYS()))
 *		return NULL;
 *	...
 * }
 *
 * @endcode
 *
 * It creates a fault point with tag "pretend_failure" in function "kvsal_alloc",
 * which can be enabled/disabled from external code with something like the
 * following:
 *
 * @code
 *
 * 	ifault_fp_id_t fp_id;
 *
 * 	fp_id.func = "kvsal_alloc";
 * 	fp_id.tag = "pretend_failure";
 *
 *	fault_enable(&fp_id);
 *
 * @endcode
 *
 * @param tag short descriptive name of fault point, usually separated by
 *            and uniquely identifies this FP within a current
 *            function
 *
 * @return    true, if FP is enabled
 * @return    false otherwise
 */

#define FP_ENABLED(tag, spec)				\
({							\
	static struct ifault_fp_t fp = {		\
		.fp_spec     = NULL,			\
 /* TODO: add some macro to automatically get name of current module */ \
		.fp_module   = "UNKNOWN",		\
		.fp_file     = __FILE__,		\
		.fp_line_num = __LINE__,		\
		.fp_func     = __func__,		\
		.fp_tag      = (tag),			\
	};						\
							\
	if (unlikely(fp.fp_state == NULL)) {		\
		ifault_register(&fp, spec);	\
		ASSERT(fp.fp_state != NULL);		\
	}						\
							\
	ifault_enabled(&fp);				\
})

#else

#define FP_ENABLED(tag, spec)	0

#endif /*< FAULT_INJECTION */

#endif /* _IFAULT_H */

