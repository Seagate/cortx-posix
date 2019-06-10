#!/bin/bash
################################################################################
# Test cases for RENAME operation
################################################################################

################################################################################
# Description: Rename a file in the root directory
# Strategy:
#   Create a regular file in the root directory,
#   lookup the file, rename it and look it up again.
#   Then, remove the renamed file.
# Expected behavior:
#   1. Lookup's are successfull.
#   2. Inode did not change.
test_kvsns_rename_file() {
    local old_name="f1"
    local new_name="f2"
    local parent="/"

    kvsns_prepare_clean_index

    test_setup kvsns_create_cmd $parent $old_name
    test_setup kvsns_lookup_cmd $parent $old_name

    test_verify_ok kvsns_rename_cmd $parent $old_name $parent $new_name
    test_verify_ok kvsns_lookup_cmd $parent $new_name
    # TODO: compare inodes

    test_teardown kvsns_unlink_cmd $parent $new_name
}

################################################################################
# Description: Rename a file in a non-root directory
# Strategy:
# Expected behavior:
test_kvsns_rename_file_non_root_dir() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
# Description: Rename a directory in the root directory
# Strategy:
# Expected behavior:
test_kvsns_rename_directory() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
# Description: Rename a non-existing file
# Strategy:
# Expected behavior:
test_kvsns_rename_noexist_file() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
# Description: Rename a dir into an empty dir.
# Strategy:
#   1. Create one dir on the source and one dir in the destination.
#   2. Move the source dir into the destination.
#   3. Check if the destination exists.
# Expected behavior:
#   The source directory successfully moved into the destination,
#   the old destination directory is remove.
test_kvsns_rename_into_empty_dir() {
    local old_name="d1"
    local new_name="d2"
    local parent_ino="$KVSNS_ROOT_INODE"

    kvsns_prepare_clean_index

    test_setup kvsns_mkdir_cmd $parent_ino $old_name
    test_setup kvsns_lookup_cmd $parent_ino $old_name
    test_setup kvsns_mkdir_cmd $parent_ino $new_name
    test_setup kvsns_lookup_cmd $parent_ino $new_name

    test_verify_ok kvsns_rename_cmd $parent_ino $old_name $parent_ino $new_name
    test_verify_ok kvsns_lookup_cmd $parent_ino $new_name

    # TODO: when rmdir is implemented
    # test_teardown kvsns_rmdir_cmd $parent $new_name
}

################################################################################
# Description: Rename a dir into an existing non-empty dir
# Strategy:
#   1. Create src dir and dst dir.
#   2. Create sub-dir in dst dir.
#   3. Try to replace dst dir with src dir.
# Expected behavior:
#   Rename failed with non-zero return code.
test_kvsns_rename_into_non_empty_dir() {
    local parent_ino="$KVSNS_ROOT_INODE"
    local src_dir="d4"
    local dst_dir="d5"
    local dst_dir_ino="5"
    local sub_dst_dir="d6"

    kvsns_prepare_clean_index

    test_setup kvsns_mkdir_cmd $parent_ino $src_dir
    test_setup kvsns_mkdir_cmd $parent_ino $dst_dir
    test_setup kvsns_mkdir_cmd $dst_dir_ino $sub_dst_dir

    test_verify_fail kvsns_rename_cmd $parent_ino $old_name $parent_ino $dst_dir

    # TODO: remove $src_dir, $dst_dir, $sub_dst_dir
}

################################################################################
# Description: Rename an open file (source)
# Strategy:
# Expected behavior:
test_kvsns_rename_file_open_src() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
# Description: Rename a file into an open file (destination)
# Strategy:
# Expected behavior:
test_kvsns_rename_file_open_dst() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
# Description: Rename a hardlink (source)
# Strategy:
# Expected behavior:
test_kvsns_rename_src_hard_link() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
# Description: Rename file into a hardlink (destination)
# Strategy:
# Expected behavior:
test_kvsns_rename_dst_hard_link() {
    echo "${FUNCNAME[0]}: NotImplemented"
    return 0
}

################################################################################
