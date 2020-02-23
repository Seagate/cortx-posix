cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
        message (FATAL ERROR "Conannot find cmocka")
endif((NOT HAVE_CMOCKA))

add_executable(ut test_nsal.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

set(nsal_TEST_SRC
        test_nsal_ns.c
)
add_library(nsal_tests SHARED ${nsal_TEST_SRC})

target_link_libraries(ut
	eos-utils
	eos-nsal
	nsal_tests
	${STORE_LIBRARY}
)
