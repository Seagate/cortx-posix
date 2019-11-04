set(MERO_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MERO_CFLAGS "${MERO_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MERO_CFLAGS "${MERO_CFLAGS} -I/usr/include/mero ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MERO_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MERO_CFLAGS}")

SET(kvsns_LIB_SRCS
    kvsns_init.c
    kvsns_handle.c
    kvsns_file.c
    kvsns_internal.c
    kvsns_xattr.c
    kvsns_copy.c
)

add_library(kvsns SHARED ${kvsns_LIB_SRCS})
target_link_libraries(kvsns ini_config ${STORE_LIBRARY} ${KVSAL_LIBRARY})

