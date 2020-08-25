
include_directories(${CORTXUTILSINC})

SET(DSAL_LIB_SRCS
   dsal.c
   dstore_bufvec.c
)

add_library(dsal OBJECT ${DSAL_LIB_SRCS})

