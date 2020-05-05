cmake_minimum_required(VERSION 2.6.3)

set(CMAKE_EV_INC /usr/local/include)
set(CMAKE_EV_LIB /usr/local/lib)

include_directories(include)
include_directories(${CMAKE_EV_INC}/event2)
include_directories(${CMAKE_EV_INC}/evhtp)
link_directories(${CMAKE_EV_LIB})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lpthread")

SET(control_LIB_SRCS
	server.c
	fs.c
	endpoint.c
)

add_library(management OBJECT ${control_LIB_SRCS})
