
include_directories(${EOSUTILSINC})

SET(KVTREE_LIB_SRCS
   kvtree.c
   kvnode.c
)

add_library(kvtree OBJECT ${KVTREE_LIB_SRCS})

