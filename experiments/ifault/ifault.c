/**
 * Filename: fault.c
 * Description: Fault injection Framework.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ifault.h>
#include <ifault-internal.h>

/**
 * fault_init:
 * Initializes Fault Framework
 */
void ifault_init(void)
{
	int rc = 0;
	unsigned int random_seed;

	ifault_table = malloc(sizeof(ifault_fp_table_t));
	assert(ifault_table != NULL);

	pthread_mutex_init(&ifault_table->fpt_lock, NULL);
	assert(rc == 0);

	ifault_table->fpt_size = IFAULT_INIT_TABLE_SIZE;

	ifault_table->fpt_list = malloc(ifault_table->fpt_size *
					sizeof(ifault_fp_state_t));
	assert(ifault_table->fpt_list != NULL);

	ifault_table->fpt_idx = 0;

	/**
 	 * Initialize pseudo random generator, which is used in FI_TYPE_RANDOM
 	 * triggering algorithm.
 	 */
	random_seed = get_pid() ^ time(NULL);
	srandom(random_seed);
}

void ifault_deinit(void)
{
	pthread_mutex_destroy(&ifault_table->fpt_lock);

	free(ifault_table->fpt_list);
	ifault_table->fpt_list = NULL;

	free(ifault_table);
	ifault_table = NULL;
}

void ifault_register(ifault_fp_t *fp, ifault_fp_type_t fp_type,
			uint32_t spec_argc, ...)
{
	va_list valist;

	ifault_fp_state_t *fp_state = NULL;
	ifault_fp_data_t *fp_data = NULL;
	ifault_fp_id_t fp_id;

	// Construct Fault Point ID.
	fp_id.fpi_func = fp->fp_func;
	fp_id.fpi_tag = fp->fp_tag;

	// Find the Fault Point State.
	ifp_state_find(&fp_id, &fp_state);
	assert(fp_state != NULL);

	// Contruct the Fault Point data.
	va_start(valist, spec_argc);

	fp_data = &fp_state->fps_data;
	ifp_data_init(fp_data, fp_type, spec_argc, valist);

	va_end(valist);

	// Attach Spec Handler and Fault Point to the State. 
	ifp_state_attach_hanlder(fp_state, fp_type);
	ifp_state_attach_fp(fp_state, fp);
}

int ifault_enabled(ifault_fp_t *fp)
{
	int rc = 0;
	ifault_fp_state_t *fp_state = NULL;
	ifault_fp_data_t *fp_data = NULL;

	fp_state = fp->fp_state;
	assert(fp_state != NULL);

	fp_data = &fp_state->fps_data;

	pthread_mutext_lock(&fp_state->fps_lock);

		if (fp_state->fps_enabled) {
			rc = fp_state->fps_handler(fp_data);
		}

	pthread_mutext_unlock(&fp_state->fps_lock);

	return rc;
}


void ifault_enable(ifault_fp_id_t *fp_id)
{
	int rc = 0;
	ifault_fp_state_t *fp_state = NULL;

	ifp_state_find(fp_id, &fp_state);
	assert(fp_state != NULL);

	ifp_state_enable(fp_state);
}

void ifault_disable(ifault_fp_id_t *fp_id)
{
	int rc = 0;
	ifault_fp_state_t *fp_state = NULL;

	ifp_state_find(fp_id, &fp_state);
	assert(fp_state != NULL);

	ifp_state_disable(fp_state);
}
