cmake_minimum_required(VERSION 2.6.3)

include(CheckIncludeFiles)
include(CheckLibraryExists)

#set(FSALSRC "/home/dev1/src/eos-nfs/src/FSAL_KVSFS" CACHE PATH "Path to FSAL installation")
#set(GANESHASRC "/home/dev1/src/nfs-ganesha/src/" CACHE PATH "Path to NFS-Ganesha Source")

message(STATUS "FSAL Path $ENV{FSALSRC}")

set(FSALSRC "" CACHE PATH "Path to KVSNS installation")
set(GANESHASRC "" CACHE PATH "Path to NFS-Ganesha Source")

IF(FSALSRC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${FSALSRC}/include -I${GANESHASRC}/include -I${GANESHASRC}/FSAL -I${GANESHASRC}/libntirpc/nt\
irpc")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -L${FSALSRC}/lib64/")
ENDIF()

set( FSAL_DESTINATION "/usr/lib64/ganesha")

check_library_exists(
  kvsns
  kvsns_mkdir
  "/usr/lib64"
  HAVE_KVSNS_LIB
  )
check_include_files("kvsns/kvsns.h" HAVE_LIBKVSNS_H)
set(HAVE_LIBKVSNS_H TRUE)

message(STATUS "FSALSRC=${FSALSRC} HAVE_LIBKVSNS_H=${HAVE_LIBKVSNS_H} HAVE_KVSNS_LIB=${HAVE_KVSNS_LIB}")

if((NOT HAVE_KVSNS_LIB) OR (NOT HAVE_LIBKVSNS_H))
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find KVSNS runtime. Disabling KVSFS build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find KVSNS runtime. Disabling KVSFS build")
    set(USE_FSAL_KVSFS OFF)
  endif(STRICT_PACKAGE)
endif((NOT HAVE_KVSNS_LIB) OR (NOT HAVE_LIBKVSNS_H))

SET(fsalkvsfs_LIB_SRCS
   fsal_internal.c
   main.c
   export.c
   handle.c
   file.c
   xattrs.c
   mds.c
   ds.c
)

add_library(fsalkvsfs SHARED ${fsalkvsfs_LIB_SRCS})

target_link_libraries(fsalkvsfs
  kvsns hiredis
)

set_target_properties(fsalkvsfs PROPERTIES VERSION 4.2.0 SOVERSION 4)

########### install files ###############

install(TARGETS fsalkvsfs COMPONENT fsal DESTINATION  ${FSAL_DESTINATION} )
