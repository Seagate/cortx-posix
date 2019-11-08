cmake_minimum_required(VERSION 2.6.3)

add_subdirectory(ns)
add_subdirectory(al)

add_library(kvsns_op STATIC
	ns/create.c
	ns/unlink.c
	ns/mkdir.c
	ns/rmdir.c
	ns/read.c
	ns/write.c
	ns/readdir.c
	ns/lookup.c
	ns/link.c
	ns/readlink.c
	al/set.c
	al/get.c
	al/del.c
	al/inc_counter.c
)

