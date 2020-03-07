cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
        message (FATAL ERROR "Conannot find cmocka")
endif((NOT HAVE_CMOCKA))

add_executable(ut test_efs.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

set(efs_TEST_SRC
        test_efs_fs.c
)
add_library(efs_tests SHARED ${efs_TEST_SRC})

target_link_libraries(ut
	eos-utils
	eos-nsal
	eos-efs
	efs_tests
	${STORE_LIBRARY}
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
