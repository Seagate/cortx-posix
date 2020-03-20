cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
        message (FATAL ERROR "Connnot find cmocka")
endif((NOT HAVE_CMOCKA))

add_executable(fs_ops efs_fs_ops.c)

add_executable(dir_ops_ut efs_dir_ops.c)
add_executable(link_ops_ut efs_link_ops.c)

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

target_link_libraries(fs_ops
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
