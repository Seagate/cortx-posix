#ifndef _NSTEST_COMMON_H
#define _NSTEST_COMMON_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

void EOS_SETUP();
void EOS_TEARDOWN();
int KVSNS_MKDIR(char *name, mode_t mode);
int KVSNS_RMDIR(char *name);

#endif
