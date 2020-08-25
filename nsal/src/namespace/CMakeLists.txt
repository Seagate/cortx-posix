
include_directories(${CORTXUTILSINC})

SET(NAMESPACE_LIB_SRCS
   ns.c
)

add_library(namespace OBJECT ${NAMESPACE_LIB_SRCS})

