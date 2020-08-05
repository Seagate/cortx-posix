
include_directories(${EOSUTILSINC})

SET(DSAL_LIB_SRCS
   dsal.c
   dstore_bufvec.c
)

add_library(dsal OBJECT ${DSAL_LIB_SRCS})

