#include "test_ns_common.h"

#define TST_NULL		NULL
#define TST_EEXIST		"tst_eexist"
#define TST_ENOENT		"tst_enoent/tst"
#define TST_ENOTDIR_FILE 	"tst_enotdir"
#define TST_ENOTDIR_DIR		"tst_enotdir/tst"
#define MODE			0755

static char long_dir[PATH_MAX + 2] = {[0 ... PATH_MAX + 1] = 'a'};
static char loop_dir[PATH_MAX] = ".";

static struct tcase {
	char *pathname;
	int exp_errno;
} TC[] = {
	{NULL, EINVAL},
	{long_dir, E2BIG},
	{TST_EEXIST, EEXIST},
	{TST_ENOENT, ENOENT},
	{loop_dir, EEXIST},
};



/*
 * DESCRIPTION
 * check kvsns_mkdir() with various error conditions that should produce
 * EFAULT, ENAMETOOLONG, EEXIST, ENOENT, ELOOP, EEXIST.
 */

void kvsns_mkdir01()
{
	int rc;
	int i;

	//TODO make a safe mkdir function if suggested.
	rc =  KVSNS_MKDIR(TST_EEXIST, MODE);

	for(i=0; i < sizeof(TC)/sizeof(TC[0]); i++) {

		rc = KVSNS_MKDIR(TC[i].pathname, MODE);
		if(abs(rc) == TC[i].exp_errno) {
			fprintf(stdout, "kvsns_mkdir() failed as expected, rc = %s\n",
							strerror(abs(rc)));
		} else {
			fprintf(stderr, "kvsns_mkdir() failed unexpectedly; expected: %d - %sn\n",
							TC[i].exp_errno, strerror(abs(rc)));
		}
	}
}

int main(int argc, char *argv[])
{
	EOS_SETUP();

	kvsns_mkdir01();

	EOS_TEARDOWN();
	return 0;
}
