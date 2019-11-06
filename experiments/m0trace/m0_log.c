#define M0_TRACE_SUBSYSTEM M0_TRACE_SUBSYS_OTHER
#include <stdio.h>
#include <lib/time.h>
#include "module/instance.h"      /* m0 */
#include "mero/init.h"           /* m0_init */
#include "lib/trace.h"
#include "lib/user_space/trace.h" /* m0_trace_set_print_context */


M0_INTERNAL int m0_time_init(void);
M0_INTERNAL void m0_time_fini(void);

int main(int argc, char **argv)
{
//    static struct m0 instance;
//    int rc;

/*  ***** Alternative Approach I *****
    m0_instance_setup(&instance);
    rc = m0_module_init(&instance.i_self, M0_LEVEL_INST_ONCE);
    if (rc != 0)
	    printf("m0_module_init failed\n");    
*/

/*  ***** Alternative Approach II *****
    rc = m0_init(&instance);
    if (rc != 0)
            printf("Cannot initialise mero\n");
*/

    m0_time_init();

    m0_trace_init();

/*  ***** Alternative Approach I *****
    m0_module_init(&instance.i_self, M0_LEVEL_INST_READY);
*/
    m0_trace_set_immediate_mask("all");
    m0_trace_set_print_context("full");
    m0_trace_set_level("DEBUG");

    M0_LOG(M0_DEBUG, "Stand alone application for m0trace logging");

    m0_trace_fini();

    m0_time_fini();

/*  ***** Alternative Approach II *****
    m0_fini();
*/

/*  ***** Alternative Approach I *****
    m0_module_fini(&instance.i_self, M0_MODLEV_NONE);
*/

    return 0;
}
