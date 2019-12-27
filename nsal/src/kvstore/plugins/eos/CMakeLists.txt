set(EOS_CFLAGS "-D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern ")
set(EOS_CFLAGS "${EOS_CFLAGS} -include config.h ")
set(EOS_CFLAGS "${EOS_CFLAGS} -Wall -Werror -Wno-attributes -Wno-unused-but-set-variable ")
set(EOS_CFLAGS "${EOS_CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer  -fno-common -fPIC ")
set(EOS_CFLAGS "${EOS_CFLAGS} -I/usr/include/mero -I/usr/include/libcfs ")
set(EOS_CFLAGS "${EOS_CFLAGS} -I/usr/include/lustre")

include_directories("/usr/include/mero")
#LIBS=-lm -lpthread -lrt -lgf_complete -lyaml -luuid -lmero 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EOS_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EOS_CFLAGS}")

SET(kvstore_LIB_SRCS
   ../../kvstore_base.c
   eos_kvstore.c
)

add_library(kvstore SHARED ${kvstore_LIB_SRCS})

target_link_libraries(kvstore
  m0common
)

add_custom_command(TARGET kvstore
		   COMMAND /usr/bin/cp libkvstore.so ../..)
