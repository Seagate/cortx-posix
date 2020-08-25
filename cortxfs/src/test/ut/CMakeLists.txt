cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/motr")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
	message (FATAL ERROR "Cannnot find cmocka")
endif((NOT HAVE_CMOCKA))

configure_file(ut_cortxfs.conf ut_cortxfs.conf COPYONLY)

add_executable(ut_cortxfs_fs_ops ut_cortxfs_fs_ops.c)
add_executable(ut_cortxfs_endpoint_ops ut_cortxfs_endpoint_ops.c)
add_executable(ut_cortxfs_dir_ops ut_cortxfs_dir_ops.c)
add_executable(ut_cortxfs_link_ops ut_cortxfs_link_ops.c)
add_executable(ut_cortxfs_rename_ops ut_cortxfs_rename_ops.c)
add_executable(ut_cortxfs_file_ops ut_cortxfs_file_ops.c)
add_executable(ut_cortxfs_attr_ops ut_cortxfs_attr_ops.c)
add_executable(ut_cortxfs_io_ops ut_cortxfs_io_ops.c)
add_executable(ut_cortxfs_io_bug_ops ut_cortxfs_io_bug_ops.c)
add_executable(ut_cortxfs_xattr_file_ops ut_cortxfs_xattr_file_ops.c)
add_executable(ut_cortxfs_xattr_dir_ops ut_cortxfs_xattr_dir_ops.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

set(ut_cortxfs_helper_SRC
	ut_cortxfs_endpoint_dummy.c
	ut_cortxfs_helper.c

)

set(ut_link_libs
	${PROJECT_NAME_BASE}-utils
	${PROJECT_NAME_BASE}-nsal
	${PROJECT_NAME_BASE}-fs
)

add_library(ut_cortxfs_helper STATIC ${ut_cortxfs_helper_SRC})

target_link_libraries(ut_cortxfs_helper
	${PROJECT_NAME_BASE}-utils
	${PROJECT_NAME_BASE}-nsal
	${PROJECT_NAME_BASE}-fs
	${STORE_LIBRARY}
)

set(ut_cortxfs_xattr_ops_SRC
	ut_cortxfs_xattr_ops.c
)

add_library(ut_cortxfs_xattr_ops STATIC ${ut_cortxfs_xattr_ops_SRC})

target_link_libraries(ut_cortxfs_xattr_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_fs_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_endpoint_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_dir_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_link_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_rename_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_file_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_attr_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_xattr_file_ops
	ut_cortxfs_xattr_ops
)

target_link_libraries(ut_cortxfs_xattr_dir_ops
	ut_cortxfs_xattr_ops
)

target_link_libraries(ut_cortxfs_io_ops
	ut_cortxfs_helper
)

target_link_libraries(ut_cortxfs_io_bug_ops
	ut_cortxfs_helper
)

set(CMAKE_EV_LIB /usr/local/lib)
link_directories(${CMAKE_EV_LIB})

add_executable(ut_cortxfs_management ut_cortxfs_management.c)

target_link_libraries(ut_cortxfs_management
	${PROJECT_NAME_BASE}-fs
	json
	event
	ssl
	crypto
	event_openssl
	evhtp
	ut_cortxfs_helper
)
