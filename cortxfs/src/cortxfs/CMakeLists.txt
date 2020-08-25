set(CORTX_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -include config.h ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(CORTX_CFLAGS "${CORTX_CFLAGS} -I/usr/include/motr -I/usr/include/libcfs ")

include_directories("/usr/include/motr")
include_directories(${CORTXUTILSINC})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORTX_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORTX_CFLAGS}")

SET(cortxfs_LIB_SRCS
   cortxfs.c
   cortxfs_fh.c
   cortxfs_internal.c
   cortxfs_ops.c
   cortxfs_fops.c
   cortxfs_xattr.c
)

add_library(cortxfs OBJECT ${cortxfs_LIB_SRCS})

