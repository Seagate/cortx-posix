
include_directories(${CORTXUTILSINC})

SET(KVTREE_LIB_SRCS
   kvtree.c
   kvnode.c
)

add_library(kvtree OBJECT ${KVTREE_LIB_SRCS})

