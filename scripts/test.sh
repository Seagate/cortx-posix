#!/bin/bash

set -e

MODULES=(utils dsal nsal efs kvsfs)
BUILD_DIR=/tmp/efs
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

	NSAL_TEST_DIR=$BUILD_DIR/build-nsal/test/ut
	NSAL_TEST_LIST=(ut_nsal_ns_ops ut_nsal_iter_ops ut_nsal_kvtree_ops)

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
	EFS_TEST_LIST=(ut_efs_endpoint_ops ut_efs_fs_ops ut_efs_dir_ops
		       ut_efs_file_ops ut_efs_link_ops ut_efs_rename_ops
		       ut_efs_attr_ops ut_efs_xattr_file_ops
		       ut_efs_xattr_dir_ops ut_efs_io_ops)

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
