#!/bin/bash
################################################################################
# Test cases for READDIR operation
################################################################################


################################################################################
# Utils

# Check if the parent dir $1 has only one entry
# and the name of the entry is $2.
readdir_check_single_file() {
    local parent_ino=$1
    local file_name=$2
    local quoted_file_name="\"$file_name\""

    local quoted_dir_list=( $(kvsns_readdir_getlist_cmd $parent_ino $file_name) )
    local dir_list_len=${#quoted_dir_list[@]}

    if (( dir_list_len != 1)) ; then
        log_error "Invalid READDIR list len (expected 1, actual $dir_list_len)"
        return 1;
    fi

    local actual_file_name_with_quotes=${quoted_dir_list[0]}

    if [ $actual_file_name_with_quotes != $quoted_file_name ] ; then
        log_error "Invalid READDIR list entry "\
"(expected $quoted_file_name, actual $actual_file_name_with_quotes)"
        return 1
    fi

    return 0
}

# Check if READDIR for $1 returns the specific list of files.
readdir_check_multiple_files() {
    local parent_ino=$1
    shift
    local file_list=($@)
    local expected_dir_list=()

    for (( i = 0; i < ${#file_list[@]}; i++)); do
        expected_dir_list[$i]=\""${file_list[$i]}"\"
    done

    local quoted_dir_list=( $(kvsns_readdir_getlist_cmd $parent_ino $file_name) )

    local actual_len=${#quoted_dir_list[@]}
    local expected_len=${#expected_dir_list[@]}

    if (( actual_len != expected_len )) ; then
        log_error "Invalid READDIR list len (expected $expected_len, actual $actual_len)"
        return 1
    fi

    for (( i = 0; i < $actual_len; i++ )); do
        local left=${quoted_dir_list[$i]}
        local right=${expected_dir_list[$i]}
        if [ $left != $right ] ; then
            log_error "Invalid READDIR list element: $i, $left != $right"
            return 1
        fi
    done

    return 0
}

################################################################################
# Description: List directory contents.
# Strategy:
#   Create a regular file in the root directory.
#   List contents of the root directory.
# Expected behavior:
#   The list has the file.
test_kvsns_readdir_root_dir() {
    local file_name="file1"
    local parent_ino="$KVSNS_ROOT_INODE"
    local parent="/"

    kvsns_prepare_clean_index

    test_setup kvsns_create_cmd $parent $file_name
    test_setup kvsns_lookup_cmd $parent $file_name

    test_verify_ok kvsns_readdir_cmd $parent_ino

    test_verify_ok readdir_check_single_file $parent_ino $file_name

    test_teardown kvsns_unlink_cmd $parent $file_name
}


################################################################################
# Description: List contents of a sub-directory
# Strategy:
#   Create a directory (sub-dir) in the root directory.
#   Create a regular file in the sub-dir.
#   Get list of sub-dir files.
# Expected behavior:
#   The list has the file.
#
# TODO: kvsns_create does not support parent_ino yet,
#       so that a directory is created instead of a file.
test_kvsns_readdir_sub_dir() {
    local file_name="file1"
    local parent="/"
    local parent_ino="$KVSNS_ROOT_INODE"
    local sub_dir="dir1"
    local sub_dir_ino=4

    kvsns_prepare_clean_index

    test_setup kvsns_mkdir_cmd $parent_ino $sub_dir
    test_setup kvsns_mkdir_cmd $sub_dir_ino $file_name

    test_verify_ok kvsns_readdir_cmd $sub_dir_ino

    test_verify_ok readdir_check_single_file $sub_dir_ino $file_name

    # test_teardown kvsns_unlink_cmd $parent $file_name
}

################################################################################
# Description: Check if files and directories are both visible in READDIR.
# Strategy:
#   Create a file and a directory in the root directory.
#   Get list of entries in the root dir.
# Expected behavior:
#   The file and the dir are in the list.
test_kvsns_readdir_file_and_dir() {
    local file_name="file1"
    local dir_name="dir1"
    local parent_ino="$KVSNS_ROOT_INODE"
    local parent="/"
    local dentry_list=($dir_name $file_name)

    kvsns_prepare_clean_index

    test_setup kvsns_create_cmd $parent $file_name
    test_setup kvsns_mkdir_cmd $parent_ino $dir_name

    test_verify_ok kvsns_readdir_cmd $parent_ino

    test_verify_ok readdir_check_multiple_files $parent_ino ${dentry_list[@]}

    test_teardown kvsns_unlink_cmd $parent $file_name
    # TODO: kvsns_rmdir
}

################################################################################
# Description: List contents of an empty directory.
# Strategy:
#   Create a file and a directory in the root directory.
#   Get list of entries in the root dir.
# Expected behavior:
#   The file and the dir are in the list.
test_kvsns_readdir_empty_dir() {
    local dir_name="dir1"
    local dir_ino=4
    local parent_ino="$KVSNS_ROOT_INODE"
    local parent="/"
    local dentry_list=()

    kvsns_prepare_clean_index

    test_setup kvsns_mkdir_cmd $parent_ino $dir_name

    test_verify_ok kvsns_readdir_cmd $dir_ino

    test_verify_ok readdir_check_multiple_files $dir_ino ${dentry_list[@]}

    # TODO: kvsns_rmdir
}

################################################################################
# Description: List contents of an empty directory.
# Strategy:
#   Create a file and a directory in the root directory.
#   Get list of entries in the root dir.
# Expected behavior:
#   The file and the dir are in the list.
test_kvsns_readdir_multiple_files() {
    local parent_ino="$KVSNS_ROOT_INODE"
    local parent="/"
    local file_list=(f{0..9})

    kvsns_prepare_clean_index

    for (( i = 0; i < ${#file_list[@]}; i++)); do
        test_setup kvsns_create_cmd $parent "${file_list[$i]}"
    done

    test_verify_ok kvsns_readdir_cmd $parent_ino

    test_verify_ok readdir_check_multiple_files $parent_ino ${file_list[@]}

    for (( i = 0; i < ${#file_list[@]}; i++)); do
        test_setup kvsns_unlink_cmd $parent "${file_list[$i]}"
    done
}

################################################################################

