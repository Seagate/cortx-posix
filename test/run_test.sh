#!/bin/bash

# set up base paths and names
KVSNS_BUILD_DIR=${KVSNS_BUILD_DIR:-/tmp/kvsns_build}
KVSNS_TEST_LOG_DIR=${KVSNS_TEST_LOG_DIR:-/var/log}
KVSNS_TEST_MAIN_LOG_FILE_NAME=kvsns_test_log.txt
SCRIPT_RUNNER_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# set up derived paths
KVSNS_SHELL_DIR="$KVSNS_BUILD_DIR/kvsns_shell"

# Additional env
TEST_DIR="$KVSNS_SHELL_DIR"
MY_DIR="$SCRIPT_RUNNER_DIR"
TEST_LOG="$KVSNS_TEST_LOG_DIR/$KVSNS_TEST_MAIN_LOG_FILE_NAME"


num_failed=0
num_passed=0

print_env() {
    echo "KVSNS_BUILD_DIR=$KVSNS_BUILD_DIR"
    echo "KVSNS_TEST_LOG_DIR=$KVSNS_TEST_LOG_DIR"
    echo "SCRIPT_RUNNER_DIR=$SCRIPT_RUNNER_DIR"
}

print_env

echo "Cleaning up old logs and mero trace files"
[[ -e "$TEST_LOG" ]] && rm "$TEST_LOG"
find $PWD -maxdepth 1 -name 'm0trace.*' -type f -size +1M -exec rm {} ';'

TEST_SCRIPTS="$MY_DIR/../src/kvsns/tests"
failed=0
passed=0

# Imports
. $SCRIPT_RUNNER_DIR/libtest.bashlib

run_tests ()
{
	. $1
	for test in "${TEST_LIST[@]}"; do
		SECONDS=0;
		if $test; then
			duration=$SECONDS;
			echo "$test passed, time:$(($duration % 60)) seconds"
			passed=$((passed + 1))
		else
			echo "$test failed"
			failed=$((failed + 1))
		fi
	done
}

echo "Tests are in $TEST_SCRIPTS"
for f in $TEST_SCRIPTS/*.sh; do
	[[ -e $f ]] || continue
	run_tests $f
done

echo "Passed: $passed, Failed: $failed"

