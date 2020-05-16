cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
	message (FATAL ERROR "Cannot find cmocka")
endif((NOT HAVE_CMOCKA))

configure_file(ut_nsal.conf ut_nsal.conf COPYONLY)
add_executable(test_ns test_ns.c)
add_executable(test_kvtree test_kvtree.c)
add_executable(test_tenant test_tenant.c)
add_executable(test_iter iter_test.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

target_link_libraries(test_ns
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)

target_link_libraries(test_tenant
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)

target_link_libraries(test_iter
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)

target_link_libraries(test_kvtree
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)
