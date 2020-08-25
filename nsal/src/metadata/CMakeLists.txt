set(MOTR_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -include config.h ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/motr -I/usr/include/libcfs ")
set(MOTR_CFLAGS "${MOTR_CFLAGS} -I/usr/include/lustre")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MOTR_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MOTR_CFLAGS}")

include_directories(${CORTXUTILSINC})

SET(METADATA_LIB_SRCS
   md_xattrs.c
   md_common.c
)

add_library(metadata OBJECT ${METADATA_LIB_SRCS})

