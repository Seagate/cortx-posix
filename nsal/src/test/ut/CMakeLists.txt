cmake_minimum_required(VERSION 2.6.3)

include_directories(include)
include_directories("/usr/include/mero")

find_library(HAVE_CMOCKA cmocka)

if((NOT HAVE_CMOCKA))
	message (FATAL ERROR "Cannot find cmocka")
endif((NOT HAVE_CMOCKA))

configure_file(ut_nsal.conf ut_nsal.conf COPYONLY)
add_executable(ut_nsal_ns_ops ut_nsal_ns_ops.c)
add_executable(ut_nsal_kvtree_ops ut_nsal_kvtree_ops.c)
add_executable(ut_nsal_iter_ops ut_nsal_iter_ops.c)

link_directories(${LIBKVSTORE})
set(CMAKE_INSTALL_RPATH "${LIBKVSTORE}")

target_link_libraries(ut_nsal_ns_ops
	${PROJECT_NAME_BASE}-utils
	${PROJECT_NAME_BASE}-nsal
	${STORE_LIBRARY}
)

target_link_libraries(ut_nsal_iter_ops
	${PROJECT_NAME_BASE}-utils
	${PROJECT_NAME_BASE}-nsal
	${STORE_LIBRARY}
)

target_link_libraries(ut_nsal_kvtree_ops
	${PROJECT_NAME_BASE}-utils
	${PROJECT_NAME_BASE}-nsal
	${STORE_LIBRARY}
)
