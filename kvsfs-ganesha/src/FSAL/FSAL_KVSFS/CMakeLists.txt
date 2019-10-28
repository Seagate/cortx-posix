cmake_minimum_required(VERSION 2.6.3)

include(CheckIncludeFiles)
include(CheckLibraryExists)

set(LIBFSALKVSFS_MAJOR_VERSION 1)
set(LIBFSALKVSFS_MINOR_VERSION 0)
set(LIBFSALKVSFS_PATCH_LEVEL 1)
set(LIBFSALKVSFS_EXTRA_VERSION ${RELEASE_VER})

set(LIBFSALKVSFS_BASE_VERSION ${BASE_VERSION})
set(NFS_SETUP_DIR ${CMAKE_SOURCE_DIR}/../conf)

################################################################################
# Required arguments

set(DEFAULT_GANESHASRC "")
set(DEFAULT_GANESHABUILD "")
set(DEFAULT_KVSNSINC "")
set(DEFAULT_LIBKVSNS "")

set(GANESHASRC ${DEFAULT_GANESHASRC} CACHE PATH "Path to NFS-Ganesha Source (nfs-ganesha/src)")
set(GANESHABUILD ${DEFAULT_GANESHABUILD} CACHE PATH "Path to NFS-Ganesha Build (nfs-ganesha/build)")
set(KVSNSINC ${DEFAULT_KVSNSINC} CACHE PATH "Path to folder with kvsns.h")
set(LIBKVSNS ${DEFAULT_LIBKVSNS} CACHE PATH "Path to folder with libkvsns.so")
set(FSAL_DESTINATION "/usr/lib64/ganesha" CACHE PATH "Target directory for FSAL")

# TODO: Add a condition here and check nfs-ganesha configuration
set(SYSTEM_LIBRARIES
	"/usr/lib64/libjemalloc.so")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${GANESHASRC}/libntirpc)
include_directories(${GANESHASRC}/libntirpc/ntirpc)
include_directories(${GANESHABUILD}/include)
include_directories(${KVSNSINC})
include_directories(${GANESHASRC}/include)
include_directories(${GANESHASRC}/include/FSAL)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable")

# TODO: Wrap this with a check against build type
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")


check_library_exists(kvsns kvsns_mkdir ${LIBKVSNS} HAVE_KVSNS_LIB)

# FIXME: it looks like this check does not work correctly --
# it checks /usr/include files only ignoring "include_directories".
# We might need to figure out something better.
check_include_files("kvsns/kvsns.h" HAVE_LIBKVSNS_H)

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

add_library(fsalkvsfs SHARED ${fsalkvsfs_LIB_SRCS})

target_link_libraries(fsalkvsfs
  kvsns
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

set(RPM_BUILD_OPTIONS " --define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}' --define '_lib_path ${CMAKE_BINARY_DIR}' --define '_nfs_setup_dir ${CMAKE_SOURCE_DIR}/conf' ")

add_custom_target( rpm DEPENDS dist)
add_custom_command(TARGET rpm
	COMMAND sh -c "rpmbuild ${RPM_BUILD_OPTIONS}  -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
	VERBATIM
	DEPENDS dist)

# A quick check of the dependencies of the library
add_custom_target(check_ldd DEPENDS fsalkvsfs)
add_custom_command(TARGET check_ldd
	COMMAND env LD_LIBRARY_PATH=${LIBKVSNS} LD_PRELOAD=/usr/bin/ganesha.nfsd /usr/bin/ldd -r ${CMAKE_BINARY_DIR}/libfsalkvsfs.so
	VERBATIM)



