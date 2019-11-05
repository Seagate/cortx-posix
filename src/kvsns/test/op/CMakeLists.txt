cmake_minimum_required(VERSION 2.6.3)

add_library(kvsns_op STATIC
	create.c
	unlink.c
	mkdir.c	
	rmdir.c
	read.c	
	write.c	
	readdir.c	
	lookup.c	
	link.c	
	readlink.c	
	set.c	
	get.c	
	del.c	
)

