#include "test_ns_common.h"

#define TESTDIR		"testdir"
#define TESTDIR2	"nosuchdir/testdir2"
#define MODE		0755

//TODO: Need to confirm validity for the follwoing
/*
 *  * Description:
 * 1) attempt to kvsns_rmdir() non-empty directory -> ENOTEMPTY
 * 2) attempt to kvsns_rmdir() directory with long path name -> ENAMETOOLONG
 * 3) attempt to kvsns_rmdir() non-existing directory -> ENOENT
 * 4) attempt to kvsns_rmdir() a file -> ENOTDIR
 * 5) attempt to kvsns_rmdir() invalid pointer -> EFAULT
 * 6) attempt to kvsns_rmdir() symlink loop -> ELOOP
 * 7) attempt to kvsns_rmdir() dir on RO mounted FS -> EROFS
 * 8) attempt to kvsns_rmdir() mount point -> EBUSY
 * 9) attempt to kvsns_rmdir() current directory "." -> EINVAL
 * */

static char longpathname[PATH_MAX + 2];

static struct testcase {
	char *dir;
	int exp_errno;
} TC[] =  {
//	{TESTDIR, ENOTEMPTY},
	{longpathname, E2BIG},
	{TESTDIR2, ENOENT},
	{".", EINVAL},
	{NULL, EINVAL},
};

void kvsns_rmdir01()
{
	int i;
	int rc;
	rc = KVSNS_MKDIR(TESTDIR, MODE);

	for(i = 0; i < sizeof(TC)/sizeof(TC[0]); i++) {
		rc = KVSNS_RMDIR(TC[i].dir);
		if(abs(rc) == TC[i].exp_errno ){
			printf("kvsns_rmdir() failed as expected, rc =%s\n",
						strerror(abs(rc)));
		} else {
			printf("kvsns_rmdir() failed unexpectedly; expected: %d - %sn\n",
						TC[i].exp_errno, strerror(abs(rc)));
		}
	}
}


int main(int argc, char *argv[])
{
	EOS_SETUP();

	kvsns_rmdir01();

	EOS_TEARDOWN();
	return 0;
}
