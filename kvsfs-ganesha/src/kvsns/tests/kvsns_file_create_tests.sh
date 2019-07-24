#!/bin/bash

TEST_FILE=Test_File

LONG_NAME=12345678901123456789012345678901234567890123456789012345678901234\
567890123456789012345678901234567890123456789012345678901234567890123456789012\
456789012345678901234567890123456789012345678901234567890123456789012345678901\
34567890123456789012345678901234567890123456789012345678901234567890234567890


test_file_create()
{
	if $TEST_DIR/kvsns_lookup $TEST_FILE >> $TEST_LOG 2>&1 ; then
		echo "File already exists before create!!, run mkfs..";
		return 1;
	fi;

	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1;
	}

	$TEST_DIR/kvsns_lookup $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to lookup $TEST_FILE $"
		return 1;
	}
}

test_file_create_longname()
{
	if $TEST_DIR/kvsns_create $LONG_NAME >> $TEST_LOG 2>&1 ; then
		echo "Create $TEST_FILE test failed"
		return 1;
	else
		return 0;
	fi;
}


test_file_create_current()
{
	TEST_FILE=.

	if $TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 ; then
		echo "Create $TEST_FILE test failed";
		return 1;
	else
		return 0;
	fi;
}

test_file_create_parent()
{
	TEST_FILE=..

	if $TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 ; then
		echo "Create $TEST_FILE test failed";
		return 1;
	else
		return 0;
	fi;
}

test_file_create_root()
{
	TEST_FILE=/

	if $TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 ; then
		echo "Create $TEST_FILE test failed";
		return 1;
	else
		return 0;
	fi;
}

