include_directories(${EOSUTILSINC})

SET(FS_LIB_SRCS
   fs.c
)

add_library(fs OBJECT ${FS_LIB_SRCS})

