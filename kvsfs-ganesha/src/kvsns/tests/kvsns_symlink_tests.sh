#!/bin/bash

LINK_NAME=Test_Link

LONG_FILENAME=123456789011234567890123456789012345678901234567890123\
45678901234567890123456789012345678901234567890123456789012345678901\
23456789012345678901245678901234567890123456789012345678901234567890

SYMLINK_CONTENT="abcdefghijklmnopqrstuvwxyz"
EMPTY_CONTENT=""

test_symlink_create()
{
	$TEST_DIR/kvsns_ln $LINK_NAME $SYMLINK_CONTENT >> $TEST_LOG 2>&1 || {
		echo "Symlink creation failed!!";
		return 1;
	}

	$TEST_DIR/kvsns_readlink $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Readlink for $LINK_NAME failed"
		return 1;
	}

	$TEST_DIR/kvsns_del $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Delete $LINK_NAME link failed"
		return 1;
	}
	return 0;
}

test_symlink_create_longname()
{
	$TEST_DIR/kvsns_ln $LONG_FILENAME $SYMLINK_CONTENT >> $TEST_LOG 2>&1 || {
		echo "Symlink creation failed!!";
		return 1;
	}

	$TEST_DIR/kvsns_readlink $LONG_FILENAME >> $TEST_LOG 2>&1 || {
		echo "Readlink for $LONG_FILENAME failed"
		return 1;
	}

	$TEST_DIR/kvsns_del $LONG_FILENAME >> $TEST_LOG 2>&1 || {
		echo "Delete $LONG_FILENAME link failed"
		return 1;
	}
	return 0;
}

test_symlink_create_no_content()
{
	$TEST_DIR/kvsns_ln $LINK_NAME $EMPTY_CONTENT >> $TEST_LOG 2>&1 && {
		echo "Symlink created!!";
		return 1;
	}
	return 0;
}
