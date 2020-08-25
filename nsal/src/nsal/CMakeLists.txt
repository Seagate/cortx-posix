
include_directories(${CORTXUTILSINC})

SET(NSAL_LIB_SRCS
   nsal.c
)

add_library(nsal OBJECT ${NSAL_LIB_SRCS})

