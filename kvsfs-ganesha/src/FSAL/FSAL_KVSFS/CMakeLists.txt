cmake_minimum_required(VERSION 2.6.3)

set(LIB_FS_GANESHA ${PROJECT_NAME_BASE}-fs-ganesha)

PROJECT(${LIB_FS_GANESHA} C)

include(CheckIncludeFiles)
include(CheckLibraryExists)

set(KVSFS_GANESHA_MAJOR_VERSION 1)
set(KVSFS_GANESHA_MINOR_VERSION 0)
set(KVSFS_GANESHA_PATCH_LEVEL 1)
set(KVSFS_GANESHA_EXTRA_VERSION ${RELEASE_VER})

set(KVSFS_GANESHA_BASE_VERSION ${BASE_VERSION})
set(NFS_SETUP_DIR ${CMAKE_SOURCE_DIR}/conf)

################################################################################
# Required arguments

set(DEFAULT_GANESHASRC "")
set(DEFAULT_GANESHABUILD "")
set(DEFAULT_CAPIINC "")
set(DEFAULT_NSALINC "")
set(DEFAULT_LIBNSAL "")
set(DEFAULT_EFSINC "")
set(DEFAULT_LIBEFS "")
set(DEFAULT_EOSUTILSINC "")
set(DEFAULT_LIBEOSUTILS "")

set(EOSUTILSINC ${DEFAULT_EOSUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBEOSUTILS ${DEFAULT_LIBEOSUTILS} CACHE PATH "Path to folder with libeos-utils.so")

set(DEFAULT_EOSUTILSINC "")
set(DEFAULT_LIBEOSUTILS "")

set(EOSUTILSINC ${DEFAULT_EOSUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBEOSUTILS ${DEFAULT_LIBEOSUTILS} CACHE PATH "Path to folder with libeos-utils.so")

set(GANESHASRC ${DEFAULT_GANESHASRC} CACHE PATH "Path to NFS-Ganesha Source (nfs-ganesha/src)")
set(GANESHABUILD ${DEFAULT_GANESHABUILD} CACHE PATH "Path to NFS-Ganesha Build (nfs-ganesha/build)")
set(FSAL_DESTINATION "/usr/lib64/ganesha" CACHE PATH "Target directory for FSAL")
set(CAPIINC ${DEFAULT_CAPIINC} CACHE PATH "Path to CAPI")
set(NSALINC ${DEFAULT_NSALINC} CACHE PATH "Path to folder with nsal headers")
set(LIBNSAL ${DEFAULT_LIBNSAL} CACHE PATH "Path to folder with libeos-nsal.so")

set(EFSINC ${DEFAULT_EFSINC} CACHE PATH "Path to folder with nsal headers")
set(LIBEFS ${DEFAULT_LIBEFS} CACHE PATH "Path to folder with libeos-nsal.so")

set(PROJECT_NAME_BASE ${PROJECT_NAME_BASE})
set(INSTALL_DIR_ROOT ${INSTALL_DIR_ROOT})

# TODO: Add a condition here and check nfs-ganesha configuration
set(SYSTEM_LIBRARIES
	"/usr/lib64/libjemalloc.so")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${GANESHASRC}/libntirpc)
include_directories(${GANESHASRC}/libntirpc/ntirpc)
include_directories(${GANESHABUILD}/include)
include_directories(${GANESHASRC}/include)
include_directories(${GANESHASRC}/include/FSAL)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CAPIINC}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable")

# TODO: Wrap this with a check against build type
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -fPIC")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")

set(CMAKE_REQUIRED_INCLUDES ${EOSUTILSINC})

CHECK_INCLUDE_FILES("fault.h" HAVE_EOS_UTILS_H)

message(STATUS "HAVE_EOS_UTILS_H=${HAVE_EOS_UTILS_H}")

if(NOT HAVE_EOS_UTILS_H)
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find EOS-UTILS runtime. Disabling KVSNS build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find EOS-UTILS runtime. Disabling KVSNS build")
    set(USE_KVSNS OFF)
  endif(STRICT_PACKAGE)
endif(NOT HAVE_EOS_UTILS_H)

include_directories(${EOSUTILSINC})
link_directories(${LIBEOSUTILS})

include_directories(${NSALINC})
link_directories(${LIBNSAL})

include_directories(${EFSINC})
link_directories(${LIBEFS})

set(CMAKE_REQUIRED_INCLUDES ${EOSUTILSINC})

CHECK_INCLUDE_FILES("fault.h" HAVE_EOS_UTILS_H)

message(STATUS "HAVE_EOS_UTILS_H=${HAVE_EOS_UTILS_H}")

if(NOT HAVE_EOS_UTILS_H)
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find EOS-UTILS runtime. Disabling KVSFS build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find EOS-UTILS runtime. Disabling KVSFS build")
    set(USE_FSAL_KVSFS OFF)
  endif(STRICT_PACKAGE)
endif(NOT HAVE_EOS_UTILS_H)

include_directories(${EOSUTILSINC})
link_directories(${LIBEOSUTILS})

add_subdirectory(config)
set(LIBCONFIG libconfig)

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

add_library(libganesha OBJECT ${fsalkvsfs_LIB_SRCS})
set(LIBGANESHA libganesha)

add_library(${LIB_FS_GANESHA} SHARED
	   $<TARGET_OBJECTS:${LIBGANESHA}>
	   $<TARGET_OBJECTS:${LIBCONFIG}>
	   )

target_link_libraries(${LIB_FS_GANESHA}
  ${PROJECT_NAME_BASE}-fs
  ${SYSTEM_LIBRARIES}
)

set_target_properties(${LIB_FS_GANESHA} PROPERTIES VERSION 4.2.0 SOVERSION 4)

########### install files ###############

install(TARGETS ${LIB_FS_GANESHA} COMPONENT fsal DESTINATION  ${FSAL_DESTINATION} )

# rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "${LIB_FS_GANESHA}-Source" )
set(CPACK_PACKAGE_VENDOR "Seagate")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NFS-Ganesha FSAL for ${PROJECT_NAME_BASE}-FS")
SET(CPACK_PACKAGE_VERSION_MAJOR ${KVSFS_GANESHA_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${KVSFS_GANESHA_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${KVSFS_GANESHA_PATCH_LEVEL})

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
  "${PROJECT_SOURCE_DIR}/${LIB_FS_GANESHA}.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_FS_GANESHA}.spec"
)

set(RPM_BUILD_OPTIONS " --define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}' --define '_lib_path ${CMAKE_BINARY_DIR}' --define '_nfs_setup_dir ${CMAKE_SOURCE_DIR}/conf' ")

add_custom_target( rpm DEPENDS dist)
add_custom_command(TARGET rpm
	COMMAND sh -c "rpmbuild ${RPM_BUILD_OPTIONS}  -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
	VERBATIM
	DEPENDS dist)

# A quick check of the dependencies of the library
add_custom_target(check_ldd DEPENDS ${LIB_FS_GANESHA})
add_custom_command(TARGET check_ldd
	COMMAND env LD_LIBRARY_PATH=${LIBEFS} LD_PRELOAD=/usr/lib64/libganesha_nfsd.so /usr/bin/ldd -r ${CMAKE_BINARY_DIR}/${LIB_FS_GANESHA}.so
	VERBATIM)

