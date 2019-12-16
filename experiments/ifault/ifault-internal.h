/**
 * Filename: ifault-internal.h
 * Description: Internal Headers for fault framework.
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

#ifndef _IFAULT_INTERNAL_H
#define _IFAULT_INTERNAL_H

#include <string.h>
#include <stdarg.h>

/**
 * User func call back.
 */
typedef int (*func_cb_t)(void *);


/**
 * Fault point data.
 */
typedef struct ifault_fp_data {
	/*< Fault point type */
	ifault_fp_type_t	fpd_type;
	/*< Number of times FP was checked */
	uint32_t		fpd_hit_count;

	union {
		struct {
			/*< Point when FP should be triggered */
			uint32_t hit_start;
			/* Point beyond which FP should not be triggered */
			uint32_t hit_end;
		} n_and_m;

		struct {
			/*< User func call back */
			func_cb_t func_cb;
			void *func_cb_data;
		} user;
		/* Delay amount FP should add */
		uint32_t delay;
		/**
		 * Probability with which FP is triggered (for M0_FI_RANDOM
		 * type), it's an integer number in range [1..100], which means
		 * a probability in percents, with which FP should be triggered
		 * on each hit
		 */
		uint32_t prob;
	} fpd_u;

} ifault_fp_data_t;

/**
 * Faunt point spec handler
 */
typedef int (*fps_handler_t)(ifault_fp_data_t *fp_data);


/**
 * ##############################################################
 * # 		Fault Point Spec handler Definations.		#
 * ##############################################################
 */

static inline int ifp_once_spec(ifault_fp_data_t *fp_data)
{
	return (fp_data->fpd_hit_count == fp_data->fpd_u.n_and_m.hit_start);
}

static inline int ifp_always_spec(ifault_fp_data_t *fp_data)
{
	return (fp_data->fpd_hit_count >= fp_data->fpd_u.n_and_m.hit_start);
}

static inline int ifp_repeat_spec(ifault_fp_data_t *fp_data)
{

	return (fp_data->fpd_hit_count >= fp_data->fpd_u.n_and_m.hit_start &&
		fp_data->fpd_hit_count <= fp_data->fpd_u.n_and_m.hit_start);
}

static inline int ifp_skip_spec(ifault_fp_data_t *fp_data)
{

	return (fp_data->fpd_hit_count <= fp_data->fpd_u.n_and_m.hit_start &&
		fp_data->fpd_hit_count >= fp_data->fpd_u.n_and_m.hit_start);
}

static inline int ifp_delay_spec(ifault_fp_data_t *fp_data)
{
	sleep(fp_data->fpd_u.delay);
	return 1;
}

#define FI_RAND_PROB_SCALE 100

static inline int ifp_random_spec(ifault_fp_data_t *fp_data)
{
	return (fp_data->fpd_u.prob >= (random() % FI_RAND_PROB_SCALE));
}

static inline int ifp_user_func_spec(ifault_fp_data_t *fp_data)
{
	return fp_data->fpd_u.user.func_cb(fp_data->fpd_u.user.func_cb_data);
}

static fps_handler_t ifault_spec_table[FP_TYPE_NR] = {
	[FP_TYPE_ONCE]		= ifp_once_spec,
	[FP_TYPE_ALWAYS]	= ifp_always_spec,
	[FP_TYPE_REPEAT]	= ifp_repeat_spec,
	[FP_TYPE_SKIP]		= ifp_skip_spec,
	[FP_TYPE_DELAY]		= ifp_delay_spec,
	[FP_TYPE_RANDOM]	= ifp_random_spec,
	[FP_TYPE_USER_FUNC]	= ifp_user_func_spec,
};

static inline fps_handler_t ifp_spec_get_handler(ifault_fp_type_t fp_type)
{
	return ifault_spec_table[fp_type];
}

/**
 * ##############################################################
 * # 		Fault Point Data handler Definations.		#
 * ##############################################################
 */

static void ifp_data_init(ifault_fp_data_t *fp_data, ifault_fp_type_t fp_type,
				uint32_t spec_argc, ...)
{
	va_list valist;

	assert(fp_data != NULL);
	memset(fp_data, 0, sizeof(ifault_fp_data_t));

	// Attach Fault point spec type.
	fp_data->fpd_type = fp_type;

	va_start(valist, spec_argc);

	switch (fp_type) {
	case FP_TYPE_ONCE:
		fp_data->fpd_u.n_and_m.hit_start = va_arg(valist, uint32_t);
		break;

	case FP_TYPE_ALWAYS:
		fp_data->fpd_u.n_and_m.hit_start = va_arg(valist, uint32_t);
		break;

	case FP_TYPE_REPEAT:
		fp_data->fpd_u.n_and_m.hit_start = va_arg(valist, uint32_t);
		fp_data->fpd_u.n_and_m.hit_end = va_arg(valist, uint32_t);
		break;
	
	case FP_TYPE_SKIP:
		fp_data->fpd_u.n_and_m.hit_start = va_arg(valist, uint32_t);
		fp_data->fpd_u.n_and_m.hit_end = va_arg(valist, uint32_t);
		break;

	case FP_TYPE_DELAY:
		fp_data->fpd_u.delay = va_arg(valist, uint32_t);
		break;
	
	case FP_TYPE_RANDOM:
		fp_data->fpd_u.prob = va_arg(valist, uint32_t);
		break;

	case FP_TYPE_USER_FUNC:
		fp_data->fpd_u.user.func_cb = va_arg(valist, func_cb_t);
		fp_data->fpd_u.user.func_cb = va_arg(valist, void*);
		break;

	default:
		assert(0);
	}

	va_end(valist);
}
/**
 * ##############################################################
 * # 		Fault Point ID Definations.			#
 * ##############################################################
 */

static int ifp_id_equal(ifault_fp_id_t *fp_id1, ifault_fp_id_t *fp_id2)
{
	return (!strcmp(fp_id1->fpi_func, fp_id2->fpi_func)) &&
		(!strcmp(fp_id1->fpi_tag, fp_id2->fpi_tag));
}

/**
 * Reference to the "state" structure, which holds information about
 * current state of fault point (e.g. enabled/disabled, triggering
 * algorithm, FP data, etc.)
 */
typedef struct ifault_fp_state {
	/*< Lock */
	pthread_mutex_t		fps_lock;
	/*< Is fault point enabled? */
	int			fps_enabled;
	/*< Fault point ID */
	ifault_fp_id_t		fps_id;
	/*< Fault point data*/
	ifault_fp_data_t	fps_data;
	/*< Fault point state handler func */
	int (*fps_handler)(ifault_fp_data_t *fp_data);
	/*< Fault point info */
	ifault_fp_t		*fps_fp;

} ifault_fp_state_t;

/**
 * Fault point table
 */
typedef struct ifault_fp_table {
	/*< Lock */
	pthread_mutex_t		fpt_lock;
	/*< Max fp_list allocation size */
	uint32_t		fpt_size;
	/*< Next available slot in fp_list */
	uint32_t		fpt_idx;
	/*< list of poiters to fault points */
	ifault_fp_state_t	*fpt_list;

} ifault_fp_table_t;

#define IFAULT_INIT_TABLE_SIZE 256
static ifault_fp_table_t *ifault_table = NULL;

/**
 * ##############################################################
 * # 		Fault Point Table Handler Definations.		#
 * ##############################################################
 */

static void ifp_table_realloc_state_list(void)
{
	ifault_fp_state_t *state_list;
	
	state_list = ifault_table->fpt_list;
	ifault_table->fpt_size *= 2;

	state_list = realloc(state_list, ifault_table->fpt_size *
					sizeof (ifault_fp_state_t));
	assert(state_list != NULL);
}

/**
 * ##############################################################
 * # 		Fault Point State Handler Definations.		#
 * ##############################################################
 */

static void ifp_state_set_id(ifault_fp_state_t *fp_state, ifault_fp_id_t *fp_id)
{
	assert(fp_state != NULL);
	assert(fp_id != NULL);

	fp_state->fps_id.fpi_func = strdup(fp_id->fpi_func);
	assert(fp_state->fps_id.fpi_func != NULL);

	fp_state->fps_id.fpi_tag = strdup(fp_id->fpi_tag);
	assert(fp_state->fps_id.fpi_tag != NULL);
}

static void ifp_state_init(ifault_fp_state_t *fp_state, ifault_fp_id_t *fp_id)
{
	int rc = 0;

	assert(fp_state != NULL);
	assert(fp_id != NULL);

	rc = pthread_mutex_init(&fp_state->fps_lock, NULL);
	assert(rc == 0);

	fp_state->fps_enabled = 0;
	fp_state->fps_fp = NULL;

	ifp_state_set_id(fp_state, fp_id);
}

static void ifp_state_find(ifault_fp_id_t *fp_id, ifault_fp_state_t **fp_state)
{
	int i = 0;
	int found = 0;

	assert(fp_id != NULL);
	assert(*fp_state != NULL);

	ifault_fp_state_t *state_list = NULL;

	pthread_mutex_lock(&ifault_table->fpt_lock);

		for (i = 0; i < ifault_table->fpt_idx; i++) {
			if (ifp_id_equal(&ifault_table->fpt_list[i].fps_id,
					fp_id)) {
				found = 1;
				*fp_state = &ifault_table->fpt_list[i];
				break;
			}
		}

		if (!found) {
			// Try to allocate new state.
			if (ifault_table->fpt_idx == ifault_table->fpt_size) {
				// Realloc fpt_list
				ifp_table_realloc_state_list();
			}

			*fp_state = &ifault_table->fpt_list[ifault_table->fpt_idx];
			ifault_table->fpt_idx++;

			// Init state
			ifp_state_init(*fp_state, fp_id);
		}

	pthread_mutex_unlock(&ifault_table->fpt_lock);
}

static int ifp_state_enable(ifault_fp_state_t *fp_state)
{
	int rc = 0;
	
	assert(fp_state != NULL);
 
	pthread_mutex_lock(&fp_state->fps_lock);

		if (fp_state->fps_enabled == 1)
			rc = 1;
		fp_state->fps_enabled = 1;

	pthread_mutex_unlock(&fp_state->fps_lock);

	return rc;
}

static int ifp_state_disable(ifault_fp_state_t *fp_state)
{
	int rc = 0;

	assert(fp_state != NULL);

	pthread_mutex_lock(&fp_state->fps_lock);

		if (fp_state->fps_enabled == 0)
			rc = 1;
		fp_state->fps_enabled = 0;

	pthread_mutex_unlock(&fp_state->fps_lock);

	return rc;
}

static void ifp_state_attach_fp(ifault_fp_state_t *fp_state, ifault_fp_t *fp)
{
	assert(fp_state != NULL);
	assert(fp != NULL);

	pthread_mutex_lock(&fp_state->fps_lock);
		
		// Attach fault point to the state.
		fp_state->fps_fp = fp;

		// Attach the state to the fault point.
		fp->fp_state = fp_state;

	pthread_mutex_unlock(&fp_state->fps_lock);
}

static void ifp_state_attach_hanlder(ifault_fp_state_t *fp_state,
					ifault_fp_type_t fp_type)
{
	assert(fp_state != NULL);

	pthread_mutex_lock(&fp_state->fps_lock);

		// Attach spec handler func.
		fp_state->fps_handler = ifp_spec_get_handler(fp_type);

	pthread_mutex_unlock(&fp_state->fps_lock);
}

#endif
