#!/bin/bash

set -e

MODULES=(utils dsal nsal efs kvsfs)
BUILD_DIR=/tmp/eos-fs
TEST_GRP=all
LOG_ROOT="/var/log/cortx"
UT_LOG_ROOT="$LOG_ROOT/test/ut"

delete_logs() {
	rm -rf $UT_LOG_ROOT
}

execute_ut_nsal () {
	echo "NSAL Unit tests"

	mkdir -p $UT_LOG_ROOT/nsal
	cd $UT_LOG_ROOT/nsal

	NSAL_TEST_DIR=$BUILD_DIR/build-nsal/test
	NSAL_TEST_LIST=(test_ns test_iter test_kvtree)

	[ ! -d $NSAL_TEST_DIR ] && echo "NSAL is not built" && exit 1

	for tests in "${NSAL_TEST_LIST[@]}"
	do
		$NSAL_TEST_DIR/$tests
		echo
	done
}

execute_ut_efs () {
	echo "EFS Unit tests"

	mkdir -p $UT_LOG_ROOT/efs
	cd $UT_LOG_ROOT/efs

	EFS_TEST_DIR=$BUILD_DIR/build-efs/test/ut
	EFS_TEST_LIST=(fs_ops_ut dir_ops_ut file_ops_ut link_ops_ut rename_ops_ut attr_ops_ut xattr_ops_ut io_ops_ut)

	[ ! -d $EFS_TEST_DIR ] && echo "EFS is not built" && exit 1

	for tests in "${EFS_TEST_LIST[@]}"
	do
		$EFS_TEST_DIR/$tests
		echo 
	done
}

execute_ut_dsal () {
	# Do nothing. To be implemented
	echo "${FUNCNAME[0]}: NotImplemented"
	return 0
}

execute_ut_utils () {
	# Do nothing. To be implemented
	echo "${FUNCNAME[0]}: NotImplemented"
	return 0
}

execute_ut_kvsfs () {
	# Do nothing. To be implemented
	echo "${FUNCNAME[0]}: NotImplemented"
	return 0
}

execute_all_ut () {
	if [ "$TEST_GRP" = "all" ]
	then
		execute_ut_nsal
		execute_ut_efs
	else
		execute_ut_$TEST_GRP
	fi
}

usage () {
	cat <<EOM
usage: $0 [-h] [-t <Test group>]
options:
	-h	Help
	-t	Test group to execute. Deafult: all groups are executed
Test group list:
nsal          NSAL unit tests
efs           EFS unit tests
EOM
exit 1
}

#main

[ $(id -u) -ne 0 ] && echo "Run this script with root privileges" && exit 1

getopt --options "ht:" --name test

while [ ! -z $1 ]; do
	case "$1" in
	-h ) usage;;
	-t ) TEST_GRP=$2; shift 1;;
	* )  usage
	esac
	shift 1
done

delete_logs
execute_all_ut
