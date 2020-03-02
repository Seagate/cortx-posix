cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
        message (FATAL ERROR "Conannot find cmocka")
endif((NOT HAVE_CMOCKA))

add_executable(test_ns test_ns.c)
add_executable(test_iter iter_test.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

target_link_libraries(test_ns
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)

target_link_libraries(test_iter
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)
