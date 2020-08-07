set(EOS_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(EOS_CFLAGS "${EOS_CFLAGS} -include config.h ")
set(EOS_CFLAGS "${EOS_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(EOS_CFLAGS "${EOS_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(EOS_CFLAGS "${EOS_CFLAGS} -I/usr/include/motr -I/usr/include/libcfs ")

include_directories("/usr/include/motr")
include_directories(${EOSUTILSINC})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EOS_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EOS_CFLAGS}")

SET(dstore_LIB_SRCS
   ../../dstore_base.c
   eos_dstore.c
)

add_library(dstore OBJECT ${dstore_LIB_SRCS})

