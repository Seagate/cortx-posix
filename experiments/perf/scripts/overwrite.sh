#!/bin/bash

prepare_source() {
    local src=$1
    local rc=0

    dd if=/dev/urandom of=$src bs=1M count=100 status=progress
    rc=$?
    if (( rc != 0 )); then
        echo "Failed to generate files"
        exit 1
    fi

    return 0
}

overwrite_file() {
    local src=$1
    local dst=$2
    local rc=0

    dd if=$src of=$dst bs=1M count=100 status=progress conv=notrunc 
    rc=$?
    if (( rc != 0 )); then
        echo "DD Failed! " $rc
        exit 1
    fi

    diff -b $src $dst
    rc=$?
    if (( rc != 0 )); then
        echo "Files are different"
        exit 1
    fi

    return 0
}


main() {
    local first=/tmp/f1
    local second=/tmp/f2
    local mnt=/mnt/kvsns
    local target=f1
    local iter=0

    if [ "x$1" != "x" ]; then
        mnt=$1
    fi
    local dst="$mnt/$target"

    prepare_source $first
    prepare_source $second

    while true; do
        echo "Enter Iteration: $(( iter++ ))"
        overwrite_file $first $dst
        overwrite_file $second $dst
        echo "Exit Iteration: $(( iter++ ))"
    done
}

main $1
