#!/bin/bash

TEST_FILE=test_file_attr
TEST_MTIME=1234567890
TEST_ATIME=4444444444
TEST_CTIME=8888888888
TEST_UID=0
TEST_GID=100

attr_test_init()
{
	if $TEST_DIR/kvsns_lookup $TEST_FILE >> $TEST_LOG 2>&1 ; then
		return 0;
	fi;

	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1;
	}
}

attr_test_cleanup()
{
	$TEST_DIR/kvsns_del $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to remove $TEST_FILE"
		return 1;
	}

}

test_ctime()
{
	attr_test_init || {
		echo "test init failed"
		return 1;
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE ctime $TEST_CTIME >> $TEST_LOG 2>&1 || {
		echo "setattr ctime=$TEST_CTIME on $TEST_FILE failed!"
		attr_test_cleanup
		return 1
	}

	attrs=$($TEST_DIR/kvsns_getattr $TEST_FILE 2>&1) || {
		echo "getattr failed!!"
		attr_test_cleanup
		return 1;
	}

	val=$(echo "$attrs" | awk '/change/ {print $5}')
	if [ "$val" != $TEST_CTIME ]
	then
		echo "ctime incorrect"
		attr_test_cleanup
		return 1;
	fi
	attr_test_cleanup
}

test_mtime()
{
	attr_test_init || {
		echo "test init failed"
		return 1;
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE mtime $TEST_MTIME >> $TEST_LOG 2>&1 || {
		echo "setattr mtime=$TEST_MTIME on $TEST_FILE failed!"
		attr_test_cleanup
		return 1
	}

	attrs=$($TEST_DIR/kvsns_getattr $TEST_FILE 2>&1) || {
		echo "getattr failed!!"
		attr_test_cleanup
		return 1;
	}

	val=$(echo "$attrs" | awk '/modification/ {print $5}')
	if [ "$val" != $TEST_MTIME ]
	then
		echo " mtime incorrect"
		attr_test_cleanup
		return 1;
	fi
	attr_test_cleanup

}

test_atime ()
{
	attr_test_init || {
		echo "test init failed"
		return 1;
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE atime $TEST_ATIME >> $TEST_LOG 2>&1 || {
		echo "setattr atime=$TEST_ATIME on $TEST_FILE failed!"
		attr_test_cleanup
		return 1
	}

	attrs=$($TEST_DIR/kvsns_getattr $TEST_FILE 2>&1) || {
		echo "getattr failed!!"
		attr_test_cleanup
		return 1;
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE gid $TEST_GID >> $TEST_LOG 2>&1 || {
		echo "setattr gid=$TEST_GID on $TEST_FILE failed!"
		attr_test_cleanup
		return 1
	}
	attr_test_cleanup
}

test_gid()
{
	attr_test_init || {
		echo "test init failed"
		return 1;
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE gid $TEST_GID >> $TEST_LOG 2>&1 || {
		echo "setattr gid=$TEST_GID on $TEST_FILE failed!"
		attr_test_cleanup
		return 1
	}

	attrs=$($TEST_DIR/kvsns_getattr $TEST_FILE 2>&1) || {
		echo "getattr failed!!"
		attr_test_cleanup
		return 1;
	}

	val=$(echo "$attrs" | awk '/group/ {print $5}')
	if [ "$val" != $TEST_GID ]
	then
		echo "gid incorrect"
		attr_test_cleanup
		return 1;
	fi
	attr_test_cleanup
}

test_uid()
{
	attr_test_init || {
		echo "test init failed"
		return 1;
	}

	$TEST_DIR/kvsns_setattr $TEST_FILE uid $TEST_UID>> $TEST_LOG 2>&1 || {
		echo "setattr uid=$TEST_UID on $TEST_FILE failed!"
		attr_test_cleanup
		return 1;
	}

	attrs=$($TEST_DIR/kvsns_getattr $TEST_FILE 2>&1) || {
		echo "getattr failed!!"
		attr_test_cleanup
		return 1;
	}

	val=$(echo "$attrs" | awk '/user/ {print $5}')
	if [ "$val" != $TEST_UID ]
	then
		echo "read_uid=$val uid incorrect"
		attr_test_cleanup
		return 1;
	fi
	attr_test_cleanup
}

