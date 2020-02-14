cmake_minimum_required(VERSION 2.6.3)

add_executable(iter_test iter_test.c)
target_link_libraries( iter_test
	eos-utils
	eos-nsal
	${STORE_LIBRARY}
)

