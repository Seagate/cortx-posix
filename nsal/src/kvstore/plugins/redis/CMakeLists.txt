SET(redis_kvstore_LIB_SRCS
   redis_kvstore.c
)

add_library(redis_kvstore OBJECT ${redis_kvstore_LIB_SRCS})

