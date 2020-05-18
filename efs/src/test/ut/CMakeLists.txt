cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
	message (FATAL ERROR "Cannnot find cmocka")
endif((NOT HAVE_CMOCKA))

configure_file(ut_efs.conf ut_efs.conf COPYONLY)

add_executable(ut_efs_fs_ops ut_efs_fs_ops.c)
add_executable(endpoint_ops_ut efs_endpoint_ops.c)
add_executable(ut_efs_dir_ops ut_efs_dir_ops.c)
add_executable(ut_efs_link_ops ut_efs_link_ops.c)
add_executable(ut_efs_rename_ops ut_efs_rename_ops.c)
add_executable(ut_efs_file_ops ut_efs_file_ops.c)
add_executable(ut_efs_attr_ops ut_efs_attr_ops.c)
add_executable(ut_efs_io_ops ut_efs_io_ops.c)
add_executable(ut_efs_io_bug_ops ut_efs_io_bug_ops.c)
add_executable(ut_efs_xattr_file_ops ut_efs_xattr_file_ops.c)
add_executable(ut_efs_xattr_dir_ops ut_efs_xattr_dir_ops.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

set(ut_efs_helper_SRC
	ut_efs_helper.c
)

add_library(ut_efs_helper STATIC ${ut_efs_helper_SRC})

target_link_libraries(ut_efs_helper
	eos-utils
	eos-nsal
	eos-efs
	${STORE_LIBRARY}
)

set(ut_efs_xattr_ops_SRC
	ut_efs_xattr_ops.c
)

add_library(ut_efs_xattr_ops STATIC ${ut_efs_xattr_ops_SRC})

target_link_libraries(ut_efs_xattr_ops
	eos-utils
	eos-nsal
	eos-efs
	ut_efs_helper
	${STORE_LIBRARY}
)

target_link_libraries(ut_efs_fs_ops
	eos-utils
	eos-nsal
	eos-efs
	${STORE_LIBRARY}
)

target_link_libraries(endpoint_ops_ut
	eos-utils
	eos-nsal
	eos-efs
	${STORE_LIBRARY}
)

target_link_libraries(ut_efs_dir_ops
	ut_efs_helper
)

target_link_libraries(ut_efs_link_ops
	ut_efs_helper
)

target_link_libraries(ut_efs_rename_ops
	ut_efs_helper
)

target_link_libraries(ut_efs_file_ops
	ut_efs_helper
)

target_link_libraries(ut_efs_attr_ops
	ut_efs_helper
)

target_link_libraries(ut_efs_xattr_file_ops
	ut_efs_xattr_ops
)

target_link_libraries(ut_efs_xattr_dir_ops
	ut_efs_xattr_ops
)

target_link_libraries(ut_efs_io_ops
	ut_efs_helper
)

target_link_libraries(ut_efs_io_bug_ops
	ut_efs_helper
)

set(CMAKE_EV_LIB /usr/local/lib)
link_directories(${CMAKE_EV_LIB})

add_executable(test-management test-management.c)

target_link_libraries(test-management
	eos-efs
	json
	event
	ssl
	crypto
	event_openssl
	evhtp
)
