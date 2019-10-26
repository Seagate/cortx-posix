cmake_minimum_required(VERSION 2.6.3)

SET(kvsns_LIB_SRCS
	cp.c
	error.c
)

add_library(kvsns_utils STATIC ${kvsns_LIB_SRCS})
