################################################################################
TGROUP_FILE_CREATE=(
	test_file_create
	test_file_create_current
	test_file_create_parent
	test_file_create_root
	test_file_create_longname
)

################################################################################
TGROUP_CRED=(
    test_uid
    test_gid
)

################################################################################
TGROUP_STAT=(
    test_ctime
    test_atime
    test_mtime
)

################################################################################
TGROUP_MKDIR=(
    test_mkdir
    test_mkdir_exist
    test_mkdir_longname
    test_mkdir_parent
    test_mkdir_current
    test_mkdir_root
)

################################################################################
TGROUP_RENAME=(
    test_kvsns_rename_file
    test_kvsns_rename_into_empty_dir
    test_kvsns_rename_into_non_empty_dir
)

################################################################################
TGROUP_READDIR=(
    test_kvsns_readdir_root_dir
    test_kvsns_readdir_sub_dir
    test_kvsns_readdir_file_and_dir
    test_kvsns_readdir_empty_dir
    test_kvsns_readdir_multiple_files
)

################################################################################
# Currently only *file_create* tests pass, because they call "delete" which
# needs to be implemented with modified binary keys and vals.
# Hence the rest of the tests are "disabled"

# @todo : Fix all the tests below and add more if required.

TEST_LIST_DISABLED=(
)

################################################################################
TEST_LIST=(
    ${TGROUP_CRED[@]}
    ${TGROUP_STAT[@]}
    ${TGROUP_FILE_CREATE[@]}
    ${TGROUP_MKDIR[@]}
    ${TGROUP_RENAME[@]}
    ${TGROUP_READDIR[@]}
)

