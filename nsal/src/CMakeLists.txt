cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

PROJECT(eos-nsal C)
set(EOS_NSAL_MAJOR_VERSION 1)
set(EOS_NSAL_MINOR_VERSION 0)
set(EOS_NSAL_PATCH_LEVEL 1)
set(EOS_NSAL_EXTRA_VERSION ${RELEASE_VER})

set(DEFAULT_EOSUTILSINC "")
set(DEFAULT_LIBEOSUTILS "")

set(EOSUTILSINC ${DEFAULT_EOSUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBEOSUTILS ${DEFAULT_LIBEOSUTILS} CACHE PATH "Path to folder with libeos-utils.so")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -DDEBUG")

set(EOS_NSAL_BASE_VERSION ${BASE_VERSION})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "eos-nsal version ${EOS_NSAL_BASE_VERSION}")


# Option (for choosing KVSAL backend)
option(USE_KVS_REDIS "Use REDIS as a KVS in KVSAL" ON)
option(USE_KVS_MERO "USE MERO as a KVS in KVSAL" OFF)

if(USE_FSAL_LUSTRE)
    set(BCOND_LUSTRE "%bcond_without")
else(USE_FSAL_LUSTRE)
    set(BCOND_LUSTRE "%bcond_with")
endif(USE_FSAL_LUSTRE)

if(USE_KVS_MERO)
  set(USE_KVS_REDIS OFF)
  message(STATUS "Disabling REDIS")
endif(USE_KVS_MERO)

# BCOND* variables for spec file */
if (USE_KVS_REDIS)
	set(BCOND_KVS_REDIS "%bcond_without")
else (USE_KVS_REDIS)
	set(BCOND_KVS_REDIS "%bcond_with")
endif (USE_KVS_REDIS)

if (USE_KVS_MERO)
	set(BCOND_KVS_MERO "%bcond_without")
else (USE_KVS_MERO)
	set(BCOND_KVS_MERO "%bcond_with")
endif (USE_KVS_MERO)

if (USE_KVS_MERO OR USE_MERO_STORE)
  set(RPM_REQUIRES "mero ${RPM_REQUIRES}")
  set(RPM_DEVEL_REQUIRES "mero-devel ${RPM_DEVEL_REQUIRES}")
endif (USE_KVS_MERO OR USE_MERO_STORE)

if (USE_KVS_REDIS)
  set(RPM_REQUIRES "redis hiredis ${RPM_REQUIRES}")
  set(RPM_DEVEL_REQUIRES "hiredis-devel ${RPM_DEVEL_REQUIRES}")
endif (USE_KVS_REDIS)

if (USE_KVS_MERO)
  set(KVS_OPT "Mero")
elseif (USE_KVS_REDIS)
  set(KVS_OPT "Redis")
endif (USE_KVS_MERO)

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

#Check EOS utils
if((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))
      message(FATAL_ERROR "Cannot find ini_config")
endif((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))

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

# Check for MERO
if(USE_KVS_MERO OR USE_MERO_STORE)
  find_library(HAVE_MERO mero)
  check_library_exists(
	mero
	m0_clovis_init
	""
	HAVE_MERO
	)
  check_include_files("mero/config.h" HAVE_MERO_H)

  find_library(HAVE_MERO_HELPERS mero-helpers)
  check_library_exists(
	mero-helpers
	m0_ufid_init
	""
	HAVE_MERO_HELPERS
	)

  if((NOT HAVE_MERO) OR (NOT HAVE_MERO_H) OR (NOT HAVE_MERO_HELPERS))
      message(FATAL_ERROR "Cannot find MERO")
  endif((NOT HAVE_MERO) OR (NOT HAVE_MERO_H) OR (NOT HAVE_MERO_HELPERS))
endif(USE_KVS_MERO OR USE_MERO_STORE)

# Print summary
message(STATUS "USE_KVS_REDIS=${USE_KVS_REDIS}")
message(STATUS "USE_KVS_MERO=${USE_KVS_MERO}")


# Build ancillary libs
add_subdirectory(kvstore)
set(KVSTORE kvstore)

include_directories(${EOSUTILSINC})
link_directories(${LIBEOSUTILS})

add_subdirectory(metadata)
set(METADATA metadata)

add_library(eos-nsal SHARED
		$<TARGET_OBJECTS:${KVSTORE}>
		$<TARGET_OBJECTS:${METADATA}>
	)

target_link_libraries(eos-nsal
  mero
  mero-helpers
  ini_config
  eos-utils
)

# CPack / rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "eos-nsal-Source" )
set(CPACK_PACKAGE_VENDOR "NSAL")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NSAL - a namespace abstraction layer")
SET(CPACK_PACKAGE_VERSION_MAJOR ${EOS_NSAL_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${EOS_NSAL_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${EOS_NSAL_PATCH_LEVEL})

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
  "${PROJECT_SOURCE_DIR}/eos-nsal.pc-in.cmake"
  "${PROJECT_SOURCE_DIR}/eos-nsal.pc"
)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/eos-nsal.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/eos-nsal.spec"
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

