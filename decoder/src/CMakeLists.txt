cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

set(DECODER ${PROJECT_NAME_BASE}-decoder)

PROJECT(${DECODER} C)
set(CORTXFS_MAJOR_VERSION 1)
set(CORTXFS_MINOR_VERSION 0)
set(CORTXFS_PATCH_LEVEL 1)
set(CORTXFS_EXTRA_VERSION ${RELEASE_VER})

set(DEFAULT_CORTXUTILSINC "")
set(DEFAULT_LIBCORTXUTILS "")

set(CORTXUTILSINC ${DEFAULT_CORTXUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBCORTXUTILS ${DEFAULT_LIBCORTXUTILS} CACHE PATH "Path to folder with libcortx-utils.so")

set(DEFAULT_LIBNSAL "")
set(LIBNSAL ${DEFAULT_LIBNSAL} CACHE PATH "Path to folder with libcortx-nsal.so")

set(DEFAULT_NSALINC "")
set(NSALINC ${DEFAULT_NSALINC} CACHE PATH "Path to folder with kvstore.h")

set(DEFAULT_LIBDSAL "")
set(LIBDSAL ${DEFAULT_LIBDSAL} CACHE PATH "Path to folder with libcortx-dsal.so")

set(DEFAULT_DSALINC "")
set(DSALINC ${DEFAULT_DSALINC} CACHE PATH "Path to folder with dstore.h")

set(DEFAULT_LIBCORTXFS "")
set(LIBCORTXFS ${DEFAULT_LIBCORTXFS} CACHE PATH "Path to folder with libcortx-nsal.so")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I\"${CMAKE_SOURCE_DIR}/include\"")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC -g -lpthread")

set(CORTX_DECODER_BASE_VERSION ${BASE_VERSION})
set(INSTALL_DIR_ROOT ${INSTALL_DIR_ROOT})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "cortx-decoder version ${CORTX_DECODER_BASE_VERSION}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

include_directories("/usr/include/motr")

include_directories(${CORTXUTILSINC})
link_directories(${LIBCORTXUTILS})

include_directories(${NSALINC})
link_directories(${LIBNSAL})

include_directories(${DSALINC})
link_directories(${LIBDSAL})

include_directories(${CORTXFSINC})
link_directories(${LIBCORTXFS})

add_subdirectory(decoder)


