cmake_minimum_required(VERSION 2.6.3)

include(CheckIncludeFiles)
include(CheckLibraryExists)

set(LIBFSALKVSFS_MAJOR_VERSION 1)
set(LIBFSALKVSFS_MINOR_VERSION 0)
set(LIBFSALKVSFS_PATCH_LEVEL 1)
set(LIBFSALKVSFS_EXTRA_VERSION ${RELEASE_VER})

set(LIBFSALKVSFS_BASE_VERSION ${LIBFSALKVSFS_MAJOR_VERSION}.${LIBFSALKVSFS_MINOR_VERSION}.${LIBFSALKVSFS_PATCH_LEVEL})

set(GANESHASRC "" CACHE PATH "Path to NFS-Ganesha Source")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -I${KVSNSINC} -I${GANESHASRC}/libntirpc/ntirpc -I${GANESHASRC}/include -I${GANESHASRC}/FSAL ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -L/usr/lib64/ -L${LIBKVSNS}")

set(FSAL_DESTINATION "/usr/lib64/ganesha")

check_library_exists(
  kvsns
  kvsns_mkdir
  ${LIBKVSNS}
  HAVE_KVSNS_LIB
  )
check_include_files("kvsns/kvsns.h" HAVE_LIBKVSNS_H)
set(HAVE_LIBKVSNS_H TRUE)

message(STATUS "HAVE_LIBKVSNS_H=${HAVE_LIBKVSNS_H} HAVE_KVSNS_LIB=${HAVE_KVSNS_LIB}")

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

set(SYSTEM_LIBRARIES "/usr/lib64/libnfsidmap.so;/usr/lib64/libjemalloc.so;/usr/lib64/libcap.so;")
set(SYSTEM_LIBRARIES "/usr/lib64/libblkid.so;/usr/lib64/libuuid.so;/usr/lib64/libdl.so;" ${SYSTEM_LIBRARIES})
set(SYSTEM_LIBRARIES "/usr/lib64/libkrb5.so;/usr/lib64/libk5crypto.so;/usr/lib64/libcom_err.so;" ${SYSTEM_LIBRARIES})
set(SYSTEM_LIBRARIES "/usr/lib64/libgssapi_krb5.so;-lpthread;/usr/lib64/librt.so;/usr/lib64/libntirpc.so.1.4;" ${SYSTEM_LIBRARIES})

add_library(fsalkvsfs SHARED ${fsalkvsfs_LIB_SRCS})

target_link_libraries(fsalkvsfs
  kvsns hiredis
  ${SYSTEM_LIBRARIES}
)

set_target_properties(fsalkvsfs PROPERTIES VERSION 4.2.0 SOVERSION 4)

########### install files ###############

install(TARGETS fsalkvsfs COMPONENT fsal DESTINATION  ${FSAL_DESTINATION} )

# rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "libfsalkvsfs" )
set(CPACK_PACKAGE_VENDOR "Seagate")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NFS-Ganesha FSAL for KVSNS")
SET(CPACK_PACKAGE_VERSION_MAJOR ${LIBFSALKVSFS_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${LIBFSALKVSFS_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${LIBFSALKVSFS_PATCH_LEVEL})

# Tell CPack the kind of packages to be generated
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

set(CPACK_SOURCE_IGNORE_FILES
  "/.git/;/.gitignore/;/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")

include(CPack)

set(PKG_NAME "${CPACK_PACKAGE_NAME}.tar.gz")
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/libfsalkvsfs.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/libfsalkvsfs.spec"
)

set(RPMDEST "--define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}'")

add_custom_target( rpm DEPENDS dist)
add_custom_command(TARGET rpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} --define \"_lib_path /tmp/fsal_build\" -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)
