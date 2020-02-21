set(EOS_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(EOS_CFLAGS "${EOS_CFLAGS} -include config.h ")
set(EOS_CFLAGS "${EOS_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(EOS_CFLAGS "${EOS_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(EOS_CFLAGS "${EOS_CFLAGS} -I/usr/include/mero -I/usr/include/libcfs ")
set(EOS_CFLAGS "${EOS_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/mero")
include_directories(${EOSUTILSINC})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EOS_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EOS_CFLAGS}")

SET(eos_kvstore_LIB_SRCS
   eos_kvstore.c
)

add_library(eos_kvstore OBJECT ${eos_kvstore_LIB_SRCS})

