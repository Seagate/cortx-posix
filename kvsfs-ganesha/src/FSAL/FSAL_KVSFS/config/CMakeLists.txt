
SET(CONFIG_LIB_SRCS
   config.c
   config_impl.c
   )

add_library(libconfig OBJECT ${CONFIG_LIB_SRCS})
