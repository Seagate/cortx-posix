set(CORTX_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -include config.h ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -I/usr/include/motr -I/usr/include/libcfs ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/mero")
include_directories(${CORTXUTILSINC})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORTX_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORTX_CFLAGS}")

SET(cortx_kvstore_LIB_SRCS
   cortx_kvstore.c
)

add_library(cortx_kvstore OBJECT ${cortx_kvstore_LIB_SRCS})

