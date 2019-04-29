#!/bin/bash

set -e

DIR=dir_foo3
LONG_DIRNAME=12345678901123456789012345678901234567890123456789012345678901234\
567890123456789012345678901234567890123456789012345678901234567890123456789012\
456789012345678901234567890123456789012345678901234567890123456789012345678901\
34567890123456789012345678901234567890123456789012345678901234567890234567890

test_mkdir_init()
{
	$TEST_DIR/kvsns_del $DIR >> $TEST_LOG 2>&1
	return 0;
}

test_mkdir()
{
	test_mkdir_init || {
		echo "test init failed"
		return 1;
	}

	$TEST_DIR/kvsns_mkdir $DIR >> $TEST_LOG 2>&1 || {
		echo "create $DIR test failed"
		return 1
	}
}

test_mkdir_exist()
{
	test_mkdir_init || {
		echo "test init failed"
		return 1;
	}
	$TEST_DIR/kvsns_mkdir $DIR >> $TEST_LOG 2>&1 || {
		echo "create $DIR test failed"
		return 1
	}

	if $TEST_DIR/kvsns_mkdir $DIR >> $TEST_LOG 2>&1 ; then
		echo "mkdir exist test failed";
		return 1;
	else
		return 0;
	fi

}

test_mkdir_longname()
{
	test_mkdir_init || {
		echo "test init failed"
		return 1;
	}

	if $TEST_DIR/kvsns_mkdir $LONG_DIRNAME >> $TEST_LOG 2>&1 ; then
		echo "Create $LONG_DIRNAME test failed";
		return 1;
	else
		return 0;
	fi

}

test_mkdir_current()
{
	test_mkdir_init || {
		echo "test init failed"
		return 1;
	}

	DIR=.
	if $TEST_DIR/kvsns_mkdir $DIR >> $TEST_LOG 2>&1 ; then
		echo "Create $DIR test failed";
		return 1;
	else
		return 0;
	fi
}

test_mkdir_parent()
{
	test_mkdir_init || {
		echo "test init failed"
		return 1;
	}

	DIR=..
	if $TEST_DIR/kvsns_mkdir $DIR >> $TEST_LOG 2>&1 ; then
		echo "Create $DIR test failed";
		return 1;
	else
		return 0;
	fi

}

test_mkdir_root()
{
	test_mkdir_init || {
		echo "test init failed"
		return 1;
	}

	DIR=/
	if $TEST_DIR/kvsns_mkdir $DIR >> $TEST_LOG 2>&1 ; then
		echo "Create $DIR test failed";
		return 1;
	else
		return 0;
	fi

}
