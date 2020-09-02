################################################################################
# Required pre-definitions
cmake_minimum_required(VERSION 2.6.3)

# Use the public headers from "../../src/include".
include_directories("${PROJECT_SOURCE_DIR}/include")

# Set up the dependency list
set(DSAL_TEST_LIBS
	${PROJECT_NAME_BASE}-dsal
	${PROJECT_NAME_BASE}-utils
	cmocka
   )

# Set up library sources
set(DSAL_TEST_LIBRARY dsal_test_lib.c)

function(add_dsal_test tname tsrc)
	add_executable(${tname} ${tsrc} ${DSAL_TEST_LIBRARY})
	target_link_libraries(${tname} ${DSAL_TEST_LIBS})
endfunction()

################################################################################
# List of tests
add_dsal_test(dsal_test_basic dsal_test_basic.c)
add_dsal_test(dsal_test_space_stats dsal_test_space_stats.c)
add_dsal_test(dsal_test_io dsal_test_io.c)

################################################################################
