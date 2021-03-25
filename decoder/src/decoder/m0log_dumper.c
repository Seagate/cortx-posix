#include <stdio.h>      /* printf */
#include <err.h>        /* err */
#include <errno.h>      /* errno */
#include <string.h>     /* strcpy, basename */
#include <sysexits.h>   /* EX_* exit codes (EX_OSERR, EX_SOFTWARE) */

#include "module/instance.h"       /* m0 */
#include "motr/init.h"             /* m0_init */
#include "lib/uuid.h"              /* m0_node_uuid_string_set */
#include "lib/getopts.h"           /* M0_GETOPTS */
#include "lib/thread.h"            /* LAMBDA */
#include "lib/string.h"            /* m0_strdup */
#include "lib/user_space/types.h"  /* bool */
#include "lib/user_space/trace.h"  /* m0_trace_parse */
#include "lib/misc.h"              /* ARRAY_SIZE */
#include "lib/trace.h"

#define DEFAULT_M0MOTR_KO_IMG_PATH  "/var/log/motr/m0motr_ko.img"
//extern const int m0trace_common2;
extern const int m0trace_common3;
extern const int m0trace_common4;

int main(int argc, char *argv[])
{
	const char *input_file_name;
	const char *output_file_name;
	const char *m0motr_ko_path        = DEFAULT_M0MOTR_KO_IMG_PATH;
	enum m0_trace_parse_flags flags   = M0_TRACE_PARSE_DEFAULT_FLAGS;
	FILE       *input_file;
	FILE       *output_file;
	int         rc=0;
	const void *magic_symbols[3];
	static struct m0 instance;

	/* prevent creation of trace file for ourselves */
	m0_trace_set_mmapped_buffer(false);

	/* we don't need a real node uuid, so we force a default
	 * one to be used instead */
	m0_node_uuid_string_set(NULL);

	rc = m0_init(&instance);
	if (rc != 0)
		return EX_SOFTWARE;

	input_file_name =  m0_strdup(argv[1]);
        output_file_name = m0_strdup(argv[2]);

        input_file = fopen(input_file_name, "r");
        if (input_file == NULL) {
                printf("Failed to open input file '%s'", input_file_name);
                rc = EX_NOINPUT;
                goto out;
        }

        output_file = fopen(output_file_name, "w");
        if (output_file == NULL) {
                printf("Failed to open output file '%s'", output_file_name);
                rc = EX_NOINPUT;
                goto out;
        }


	magic_symbols[0] = (const void *)&m0trace_common2;
	magic_symbols[1] = (const void *)&m0trace_common3;
	magic_symbols[2] = (const void *)&m0trace_common4;

	rc = m0_trace_parse(input_file, output_file, m0motr_ko_path, flags,
			magic_symbols, 3);
	if (rc != 0) {
		warnx("Error occurred while parsing input trace data");
		rc = EX_SOFTWARE;
	}

	m0_fini();
out:
	fclose(output_file);
	fclose(input_file);

	return rc;
}


