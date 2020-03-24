cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

PROJECT(eos-dsal C)
set(EOS_DSAL_MAJOR_VERSION 1)
set(EOS_DSAL_MINOR_VERSION 0)
set(EOS_DSAL_PATCH_LEVEL 1)
set(EOS_DSAL_EXTRA_VERSION ${RELEASE_VER})

set(DEFAULT_EOSUTILSINC "")
set(DEFAULT_LIBEOSUTILS "")

set(EOSUTILSINC ${DEFAULT_EOSUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBEOSUTILS ${DEFAULT_LIBEOSUTILS} CACHE PATH "Path to folder with libeos-utils.so")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -fPIC -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -DDEBUG")

set(EOS_DSAL_BASE_VERSION ${BASE_VERSION})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "eos-dsal version ${EOS_DSAL_BASE_VERSION}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

option(USE_POSIX_STORE "Use POSIX directory as object store" ON)
option(USE_EOS_STORE "USE EOS as object store" OFF)
option(USE_POSIX_OBJ "USE POSIX with objs and keys" OFF)

if (USE_POSIX_STORE)
        set(BCOND_POSIX_STORE "%bcond_without")
else (USE_POSIX_STORE)
        set(BCOND_POSIX_STORE "%bcond_with")
endif (USE_POSIX_STORE)

if (USE_POSIX_OBJ)
        set(BCOND_POSIX_OBJ "%bcond_without")
else (USE_POSIX_OBJ)
        set(BCOND_POSIX_OBJ "%bcond_with")
endif (USE_POSIX_OBJ)

if (USE_EOS_STORE)
        set(BCOND_EOS_STORE "%bcond_without")
else (USE_EOS_STORE)
        set(BCOND_EOS_STORE "%bcond_with")
endif (USE_EOS_STORE)

# Final tuning
if (USE_POSIX_OBJ OR USE_EOS_STORE)
  set(USE_POSIX_STORE OFF)
  message(STATUS "Disabling POSIX Store")
endif(USE_POSIX_OBJ OR USE_EOS_STORE)

if (USE_EOS_STORE)
  set(RPM_REQUIRES "mero ${RPM_REQUIRES}")
  set(RPM_DEVEL_REQUIRES "mero-devel ${RPM_DEVEL_REQUIRES}")
endif (USE_EOS_STORE)

if (USE_EOS_STORE)
  set(EXTSTORE_OPT "EOS")
elseif (USE_POSIX_STORE)
  set(EXTSTORE_OPT "Posix")
endif (USE_EOS_STORE)


## Check ini_config
check_include_files("ini_config.h" HAVE_INI_CONFIG_H)
find_library(HAVE_INI_CONFIG ini_config)
check_library_exists(
	ini_config
	config_from_file
	""
	HAVE_INI_CONFIG
	)

if((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))
      message(FATAL_ERROR "Cannot find ini_config")
endif((NOT HAVE_INI_CONFIG) OR (NOT HAVE_INI_CONFIG_H))

# Check for EOS
if(USE_EOS_STORE)
  find_library(HAVE_EOS mero)
  check_library_exists(
	mero
	m0_clovis_init
	""
	HAVE_EOS
	)
  check_include_files("mero/config.h" HAVE_EOS_H)

  find_library(HAVE_EOS_HELPERS mero-helpers)
  check_library_exists(
	mero-helpers
	m0_ufid_init
	""
	HAVE_EOS_HELPERS
	)

  if((NOT HAVE_EOS) OR (NOT HAVE_EOS_H) OR (NOT HAVE_EOS_HELPERS))
      message(FATAL_ERROR "Cannot find EOS")
  endif((NOT HAVE_EOS) OR (NOT HAVE_EOS_H) OR (NOT HAVE_EOS_HELPERS))
endif(USE_EOS_STORE)

message(STATUS "USE_POSIX_STORE=${USE_POSIX_STORE}")
message(STATUS "USE_EOS_STORE=${USE_EOS_STORE}")
message(STATUS "USE_POSIX_OBJ=${USE_POSIX_OBJ}")

add_subdirectory(dstore)
set(DSTORE dstore)

add_subdirectory(dsal)
set(DSAL dsal)


set(CMAKE_REQUIRED_INCLUDES ${EOSUTILSINC})

CHECK_INCLUDE_FILES("fault.h" HAVE_EOS_UTILS_H)

message(STATUS "HAVE_EOS_UTILS_H=${HAVE_EOS_UTILS_H}")

if(NOT HAVE_EOS_UTILS_H)
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find EOS-UTILS runtime. Disabling DSAL build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find EOS-UTILS runtime. Disabling DSAL build")
    set(USE_DSAL OFF)
  endif(STRICT_PACKAGE)
endif(NOT HAVE_EOS_UTILS_H)

include_directories(${EOSUTILSINC})
link_directories(${LIBEOSUTILS})

add_library(eos-dsal SHARED
		$<TARGET_OBJECTS:${DSTORE}>
		$<TARGET_OBJECTS:${DSAL}>
	)

target_link_libraries(eos-dsal
  mero
  mero-helpers
  ini_config
  eos-utils
)

# CPack / rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "eos-dsal-Source" )
set(CPACK_PACKAGE_VENDOR "DSAL")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DSAL - the data abstraction layer")
SET(CPACK_PACKAGE_VERSION_MAJOR ${EOS_DSAL_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${EOS_DSAL_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${EOS_DSAL_PATCH_LEVEL})

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
  "${PROJECT_SOURCE_DIR}/eos-dsal.pc-in.cmake"
  "${PROJECT_SOURCE_DIR}/eos-dsal.pc"
)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/eos-dsal.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/eos-dsal.spec"
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

