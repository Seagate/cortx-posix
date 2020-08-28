cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

set(LIB_NSAL ${PROJECT_NAME_BASE}-nsal)

PROJECT(${LIB_NSAL} C)
set(CORTX_NSAL_MAJOR_VERSION 1)
set(CORTX_NSAL_MINOR_VERSION 0)
set(CORTX_NSAL_PATCH_LEVEL 1)
set(CORTX_NSAL_EXTRA_VERSION ${RELEASE_VER})

set(DEFAULT_CORTXUTILSINC "")
set(DEFAULT_LIBCORTXUTILS "")

set(CORTXUTILSINC ${DEFAULT_CORTXUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBCORTXUTILS ${DEFAULT_LIBCORTXUTILS} CACHE PATH "Path to folder with libcortx-utils.so")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC -g")

set(CORTX_NSAL_BASE_VERSION ${BASE_VERSION})
set(INSTALL_DIR_ROOT ${INSTALL_DIR_ROOT})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "cortx-nsal version ${CORTX_NSAL_BASE_VERSION}")

string(REPLACE "," ";" KVS_LIST "${KVS_LIST}")

foreach(KVS_ELE ${KVS_LIST})
       if(KVS_ELE STREQUAL "cortx")
               set(USE_KVS_CORTX ON)
       endif(KVS_ELE STREQUAL "cortx")

       if(KVS_ELE STREQUAL "redis")
               set(USE_KVS_REDIS ON)
       endif(KVS_ELE STREQUAL "redis")
endforeach(KVS_ELE ${KVS_LIST})

if(USE_FSAL_LUSTRE)
    set(BCOND_LUSTRE "%bcond_without")
else(USE_FSAL_LUSTRE)
    set(BCOND_LUSTRE "%bcond_with")
endif(USE_FSAL_LUSTRE)

if(NOT USE_KVS_REDIS)
  message(STATUS "Disabling REDIS")
endif(NOT USE_KVS_REDIS)

# BCOND* variables for spec file */
if (USE_KVS_REDIS)
	set(BCOND_KVS_REDIS "%bcond_without")
else (USE_KVS_REDIS)
	set(BCOND_KVS_REDIS "%bcond_with")
endif (USE_KVS_REDIS)

if (USE_KVS_CORTX)
	set(BCOND_KVS_CORTX "%bcond_without")
else (USE_KVS_CORTX)
	set(BCOND_KVS_CORTX "%bcond_with")
endif (USE_KVS_CORTX)

if (USE_KVS_CORTX OR USE_MOTR_STORE)
  set(RPM_REQUIRES "motr ${RPM_REQUIRES}")
  set(RPM_DEVEL_REQUIRES "motr-devel ${RPM_DEVEL_REQUIRES}")
endif (USE_KVS_CORTX OR USE_MOTR_STORE)

if (USE_KVS_REDIS)
  set(RPM_REQUIRES "redis hiredis ${RPM_REQUIRES}")
  set(RPM_DEVEL_REQUIRES "hiredis-devel ${RPM_DEVEL_REQUIRES}")
endif (USE_KVS_REDIS)

if (USE_KVS_CORTX)
  set(KVS_OPT "Motr")
elseif (USE_KVS_REDIS)
  set(KVS_OPT "Redis")
endif (USE_KVS_CORTX)

# Option (To enable/disable ENABLE_DASSERT mode.)
option(ENABLE_DASSERT "Enable ENABLE_DASSERT mode." ON)

if (ENABLE_DASSERT)
        set(BCOND_ENABLE_DASSERT "%bcond_without")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_DASSERT")
else (ENABLE_DASSERT)
        set(BCOND_ENABLE_DASSERT "%bcond_with")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif (ENABLE_DASSERT)

message( STATUS "ENABLE_DASSERT : ${ENABLE_DASSERT}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

## Check ini_config
check_include_files("ini_config.h" HAVE_INI_CONFIG_H)
find_library(HAVE_INI_CONFIG ini_config)
check_library_exists(
	ini_config
	config_from_file
	""
	HAVE_INI_CONFIG
	)

#Check CORTX utils
if((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))
      message(FATAL_ERROR "Cannot find ini_config")
endif((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))

set(CMAKE_REQUIRED_INCLUDES ${CORTXUTILSINC})

CHECK_INCLUDE_FILES("fault.h" HAVE_CORTX_UTILS_H)

message(STATUS "HAVE_CORTX_UTILS_H=${HAVE_CORTX_UTILS_H}")

if(NOT HAVE_CORTX_UTILS_H)
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find CORTX-UTILS runtime. Disabling CORTXFS build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find CORTX-UTILS runtime. Disabling CORTXFS build")
    set(USE_CORTXFS OFF)
  endif(STRICT_PACKAGE)
endif(NOT HAVE_CORTX_UTILS_H)

include_directories(${CORTXUTILSINC})
link_directories(${LIBCORTXUTILS})

## Start building stuff ##
if(USE_KVS_REDIS)
  find_library(HAVE_HIREDIS hiredis)
  check_library_exists(
    hiredis
    redisCommand
    ""
    HAVE_HIREDIS
    )
  check_include_files("hiredis/hiredis.h" HAVE_HIREDIS_H)

  if((NOT HAVE_HIREDIS) OR (NOT HAVE_HIREDIS_H))
      message(FATAL_ERROR "Cannot find hiredis")
  endif((NOT HAVE_HIREDIS) OR (NOT HAVE_HIREDIS_H))
endif(USE_KVS_REDIS)

# Check for CORTX
if(USE_KVS_CORTX OR USE_MOTR_STORE)
  find_library(HAVE_MOTR motr)
  check_library_exists(
	motr
	m0_client_init
	""
	HAVE_MOTR
	)
  check_include_files("motr/config.h" HAVE_MOTR_H)

  find_library(HAVE_MOTR_HELPERS motr-helpers)
  check_library_exists(
	motr-helpers
	m0_ufid_init
	""
	HAVE_MOTR_HELPERS
	)
  if((NOT HAVE_MOTR) OR (NOT HAVE_MOTR_H) OR (NOT HAVE_MOTR_HELPERS))
      message(FATAL_ERROR "Cannot find MOTR")
  endif((NOT HAVE_MOTR) OR (NOT HAVE_MOTR_H) OR (NOT HAVE_MOTR_HELPERS))
endif(USE_KVS_CORTX OR USE_MOTR_STORE)

# Print summary
message(STATUS "USE_KVS_REDIS=${USE_KVS_REDIS}")
message(STATUS "USE_KVS_CORTX=${USE_KVS_CORTX}")


# Build ancillary libs
add_subdirectory(kvstore)

set(BASE_KVSTORE base_kvstore)

set(CORTX_KVSTORE cortx_kvstore)

if(USE_KVS_REDIS)
       set(REDIS_KVSTORE redis_kvstore)
       set(REDIS_TARGET_OBJECT "$<TARGET_OBJECTS:${REDIS_KVSTORE}>")
       set(HIREDIS "hiredis")
endif(USE_KVS_REDIS)

include_directories(${CORTXUTILSINC})
link_directories(${LIBCORTXUTILS})

add_subdirectory(metadata)
set(METADATA metadata)

add_subdirectory(namespace)
set(NAMESPACE namespace)

add_subdirectory(nsal)
set(NSAL nsal)

add_subdirectory(kvtree)
set(KVTREE kvtree)

add_subdirectory(tenant)
set(TENANT tenant)

add_library(${LIB_NSAL} SHARED
		$<TARGET_OBJECTS:${BASE_KVSTORE}>
		$<TARGET_OBJECTS:${CORTX_KVSTORE}>
		${REDIS_TARGET_OBJECT}
		$<TARGET_OBJECTS:${METADATA}>
		$<TARGET_OBJECTS:${NAMESPACE}>
		$<TARGET_OBJECTS:${NSAL}>
		$<TARGET_OBJECTS:${KVTREE}>
		$<TARGET_OBJECTS:${TENANT}>
	)

target_link_libraries(${LIB_NSAL}
  motr
  motr-helpers
  ini_config
  ${PROJECT_NAME_BASE}-utils
  cmocka
  ${HIREDIS}
)

add_subdirectory(test)

# CPack / rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "(${LIB_NSAL}-Source" )
set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME_BASE}-NSAL")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NSAL - a namespace abstraction layer")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CORTX_NSAL_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${CORTX_NSAL_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${CORTX_NSAL_PATCH_LEVEL})

# Tell CPack the kind of packages to be generated
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

set(CPACK_SOURCE_IGNORE_FILES
  "/.git/;/.gitignore/;/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")

include(CPack)

set(PKG_NAME "${CPACK_PACKAGE_NAME}.tar.gz")
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# Create the .pc file
configure_file(
  "${PROJECT_SOURCE_DIR}/${LIB_NSAL}.pc-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_NSAL}.pc"
)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/${LIB_NSAL}.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_NSAL}.spec"
)

set(RPMDEST "--define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}'")

add_custom_target( rpms DEPENDS dist)
add_custom_command(TARGET rpms
                  COMMAND sh -c "rpmbuild ${RPMDEST} -ta ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

add_custom_target( rpm DEPENDS dist)
add_custom_command(TARGET rpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

add_custom_target( srpm DEPENDS dist)
add_custom_command(TARGET srpm
                  COMMAND sh -c "rpmbuild ${RPMDEST} -ts ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
                  VERBATIM
                  DEPENDS dist)

