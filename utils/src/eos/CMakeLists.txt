set(CORTX_FS_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(CORTX_FS_CFLAGS "${CORTX_FS_CFLAGS} -include config.h ")
set(CORTX_FS_CFLAGS "${CORTX_FS_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ") 
set(CORTX_FS_CFLAGS "${CORTX_FS_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ") 
set(CORTX_FS_CFLAGS "${CORTX_FS_CFLAGS} -I/usr/include/mero -I/usr/include/libcfs ")
#set(CORTX_FS_CFLAGS "${CORTX_FS_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/mero")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORTX_FS_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORTX_FS_CFLAGS}")

SET(CORTX_FS_LIB_SRCS
   m0common.c
   m0kvs.c
   m0store.c
)

add_library(utils-cortx-fs OBJECT ${CORTX_FS_LIB_SRCS})

