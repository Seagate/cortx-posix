cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
	message (FATAL ERROR "Connnot find cmocka")
endif((NOT HAVE_CMOCKA))

configure_file(ut_efs.conf ut_efs.conf COPYONLY)

add_executable(fs_ops_ut efs_fs_ops.c)
add_executable(endpoint_ops_ut efs_endpoint_ops.c)
add_executable(dir_ops_ut efs_dir_ops.c)
add_executable(link_ops_ut efs_link_ops.c)
add_executable(rename_ops_ut efs_rename_ops.c)
add_executable(file_ops_ut efs_file_ops.c)
add_executable(attr_ops_ut efs_attr_ops.c)
add_executable(xattr_ops_ut efs_xattr_ops.c)
add_executable(io_ops_ut efs_io_ops.c)
add_executable(io_ops_bug_ut efs_io_ops_bugs.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

set(efs_ut_helper_SRC
	efs_ut_helper.c
)
add_library(efs_ut_helper STATIC ${efs_ut_helper_SRC})

target_link_libraries(efs_ut_helper
	eos-utils
	eos-nsal
	eos-efs
	${STORE_LIBRARY}
)

target_link_libraries(fs_ops_ut
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

target_link_libraries(dir_ops_ut
	efs_ut_helper
)

target_link_libraries(link_ops_ut
	efs_ut_helper
)

target_link_libraries(rename_ops_ut
	efs_ut_helper
)

target_link_libraries(file_ops_ut
	efs_ut_helper
)

target_link_libraries(attr_ops_ut
	efs_ut_helper
)

target_link_libraries(xattr_ops_ut
	efs_ut_helper
)

target_link_libraries(io_ops_ut
	efs_ut_helper
)

target_link_libraries(io_ops_bug_ut
	efs_ut_helper
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
