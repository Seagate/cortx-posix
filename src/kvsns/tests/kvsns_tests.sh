#!/bin/bash

set -e
TEST_DIR=/tmp/kvsns_build/kvsns_shell
TEST_LOG=/var/log/kvsns_test_log.txt
TEST_FILE=test_foo
TEST_MTIME=1234567890
TEST_ATIME=4444444444
TEST_CTIME=8888888888
TEST_UID=10
TEST_GID=100


usage () {
	cat << EOF
usage: $me -<command>
	Commands:
	 g	run setattr/getattr tests

EOF
}

if [ $# -eq 0 ]; then
	echo "No arguments provided"
	usage
	exit 1
fi

if [[ -e $TEST_LOG ]]; then
       echo "cleaning old logs"
       rm -rfv $TEST_LOG
fi

echo "Test logs location: $TEST_LOG"

run_attr_tests() {

	local attrs;
	local val;

	$TEST_DIR/kvsns_init >> $TEST_LOG 2>&1 || {
		echo "Failed to init kvsns ns"
		return 1
	}

	$TEST_DIR/kvsns_del $TEST_FILE >> $TEST_LOG 2>&1


	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE uid $TEST_UID>> $TEST_LOG 2>&1 || {
		echo "setattr uid=$TEST_UID on $TEST_FILE failed!"
		return 1
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE gid $TEST_GID >> $TEST_LOG 2>&1 || {
		echo "setattr gid=$TEST_GID on $TEST_FILE failed!"
		return 1
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE atime $TEST_ATIME >> $TEST_LOG 2>&1 || {
		echo "setattr atime=$TEST_ATIME on $TEST_FILE failed!"
		return 1
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE mtime $TEST_MTIME >> $TEST_LOG 2>&1 || {
		echo "setattr mtime=$TEST_MTIME on $TEST_FILE failed!"
		return 1
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE ctime $TEST_CTIME >> $TEST_LOG 2>&1 || {
		echo "setattr ctime=$TEST_CTIME on $TEST_FILE failed!"
		return 1
	}


	$TEST_DIR/kvsns_getattr $TEST_FILE  >> $TEST_LOG 2>&1 || {
		echo "setattr ctime=$TEST_TIME on $TEST_FILE failed!"
		return 1
	}


	attrs=$($TEST_DIR/kvsns_getattr $TEST_FILE 2>&1) || {
		echo "getattr failed!!"
		return 1;
	}

	val=$(echo "$attrs" | awk '/access/ {print $5}')
	if [ "$val" != $TEST_ATIME ]
	then
		echo "atime incorrect"
		return 1;
	fi

	val=$(echo "$attrs" | awk '/modification/ {print $5}')
	if [ "$val" != $TEST_MTIME ]
	then
		echo " mtime incorrect"
		return 1;
	fi

	val=$(echo "$attrs" | awk '/change/ {print $5}')
	if [ "$val" != $TEST_CTIME ]
	then
		echo "ctime incorrect"
		return 1;
	fi

	val=$(echo "$attrs" | awk '/user/ {print $5}')
	if [ "$val" != $TEST_UID ]
	then
		echo "uid incorrect"
		return 1;
	fi

	val=$(echo "$attrs" | awk '/group/ {print $5}')
	if [ "$val" != $TEST_GID ]
	then
		echo "gid incorrect"
		return 1;
	fi

	echo "[set,get]attr tests passed successfully"
	return 0;
}

usage () {
	cat << EOF
usage: $me -<command>
	Commands:
	 g	run setattr/getattr tests

EOF
}

while getopts "gs" opt; do
        case $opt in
	g)
               echo "Running [set,get]attr tests"
               run_attr_tests || {
		echo "Test failed"
		}
		;;
        *)
                usage
                ;;
        esac
done

