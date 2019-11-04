set(MERO_CFLAGS "-D_REENTRANT -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MERO_CFLAGS "${MERO_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MERO_CFLAGS "${MERO_CFLAGS} -I/usr/include/mero ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MERO_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MERO_CFLAGS}")

cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

add_executable(kvsns_attach kvsns_attach.c)
target_link_libraries(kvsns_attach kvsns ${KVSAL_LIBRARY} ${STORE_LIBRARY})

