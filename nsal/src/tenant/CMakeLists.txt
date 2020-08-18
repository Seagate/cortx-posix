
include_directories(${CORTXUTILSINC})

SET(TENANT_LIB_SRCS
   tenant.c
)

add_library(tenant OBJECT ${TENANT_LIB_SRCS})

