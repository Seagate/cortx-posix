
#!/usr/bin/bash

TEST_DIR=/tmp/kvsns_build/kvsns_shell
TEST_LOG=/var/log/kvsns_test_log.txt
num_failed=0
num_passed=0


MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo $MY_DIR

echo "Cleaning up old logs and mero trace files"
rm $TEST_LOG
rm $MY_DIR/m0trace.*

TEST_SCRIPTS=$MY_DIR/../src/kvsns/tests
failed=0
passed=0


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

