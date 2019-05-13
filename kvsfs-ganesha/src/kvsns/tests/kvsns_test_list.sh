# Currently only *file_create* tests pass, because they call "delete" which
# needs to be implemented with modified binary keys and vals.
# Hence the rest of the tests are "disabled"

# @todo : Fix all the tests below and add more if required.

TEST_LIST=(
	test_file_create
	test_file_create_current
	test_file_create_parent
	test_file_create_root
	test_file_create_longname
)

TEST_LIST_DISABLED=(
	  test_uid
	  test_gid
          test_ctime
          test_atime
          test_mtime
          test_mkdir
          test_mkdir_exist
          test_mkdir_longname
          test_mkdir_parent
          test_mkdir_current
          test_mkdir_root
)

