#!/bin/bash

LINK_NAME=Test_Link
TEST_FILE=Test_File

LONG_LINKNAME=123456789011234567890123456789012345678901234567890123\
45678901234567890123456789012345678901234567890123456789012345678901\
23456789012345678901245678901234567890123456789012345678901234567890

test_hardlink_create()
{
	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1;
	}

	$TEST_DIR/kvsns_link $TEST_FILE $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Hardlink creation failed!!";
		return 1;
	}

	$TEST_DIR/kvsns_del $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Delete $LINK_NAME link failed"
		return 1;
	}

	$TEST_DIR/kvsns_del $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Delete $TEST_FILE link failed"
		return 1;
	}
	return 0;
}

test_hardlink_create_longname()
{
	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1;
	}

	$TEST_DIR/kvsns_link $TEST_FILE $LONG_LINKNAME >> $TEST_LOG 2>&1 || {
		echo "Hardlink creation failed!!";
		return 1;
	}

	$TEST_DIR/kvsns_del $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Delete $LINK_NAME link failed"
		return 1;
	}

	$TEST_DIR/kvsns_del $LONG_LINKNAME >> $TEST_LOG 2>&1 || {
		echo "Delete $LONG_LINKNAME link failed"
		return 1;
	}
	return 0;
}

test_hardlink_delete_original()
{
	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1;
	}

	$TEST_DIR/kvsns_link $TEST_FILE $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Hardlink creation failed!!";
		return 1;
	}

	$TEST_DIR/kvsns_del $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Delete $TEST_FILE_NAME link failed"
		return 1;
	}

	$TEST_DIR/kvsns_getattr $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Getattr for $LINK_NAME failed"
		return 1;
	}

	$TEST_DIR/kvsns_del $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Delete $LINK_NAME link failed"
		return 1;
	}
	return 0;
}

test_hardlink_delete_link()
{
	$TEST_DIR/kvsns_create $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Failed to create $TEST_FILE"
		return 1;
	}

	$TEST_DIR/kvsns_link $TEST_FILE $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Hardlink creation failed!!";
		return 1;
	}

	$TEST_DIR/kvsns_del $LINK_NAME >> $TEST_LOG 2>&1 || {
		echo "Delete $LINK_NAME link failed"
		return 1;
	}

	$TEST_DIR/kvsns_getattr $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Getattr for $TEST_FILE failed"
		return 1;
	}

	$TEST_DIR/kvsns_del $TEST_FILE >> $TEST_LOG 2>&1 || {
		echo "Delete $TEST_FILE link failed"
		return 1;
	}
	return 0;
}
