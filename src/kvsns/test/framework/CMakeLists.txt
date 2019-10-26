cmake_minimum_required(VERSION 2.6.3)

SET(kvsns_LIB_SRCS
	eosfs.c
	option.c
	command.c
)

add_library(kvsns_framework STATIC ${kvsns_LIB_SRCS})
