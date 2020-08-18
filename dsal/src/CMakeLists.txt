cmake_minimum_required(VERSION 2.6.3)
cmake_policy(SET CMP0017 NEW)

set(LIB_DSAL ${PROJECT_NAME_BASE}-dsal)

PROJECT(${LIB_DSAL} C)
set(CORTX_DSAL_MAJOR_VERSION 1)
set(CORTX_DSAL_MINOR_VERSION 0)
set(CORTX_DSAL_PATCH_LEVEL 1)
set(CORTX_DSAL_EXTRA_VERSION ${RELEASE_VER})

set(DEFAULT_CORTXUTILSINC "")
set(DEFAULT_LIBCORTXUTILS "")

set(CORTXUTILSINC ${DEFAULT_CORTXUTILSINC} CACHE PATH "Path to folder with fault.h")
set(LIBCORTXUTILS ${DEFAULT_LIBCORTXUTILS} CACHE PATH "Path to folder with libcortx-utils.so")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC -g")

set(CORTX_DSAL_BASE_VERSION ${BASE_VERSION})
set(CORTX_NSAL_BASE_VERSION ${BASE_VERSION})

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )
message( STATUS "cortx-dsal version ${CORTX_DSAL_BASE_VERSION}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

option(USE_POSIX_STORE "Use POSIX directory as object store" ON)
option(USE_CORTX_STORE "USE CORTX as object store" OFF)
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

if (USE_CORTX_STORE)
        set(BCOND_CORTX_STORE "%bcond_without")
else (USE_CORTX_STORE)
        set(BCOND_CORTX_STORE "%bcond_with")
endif (USE_CORTX_STORE)

# Final tuning
if (USE_POSIX_OBJ OR USE_CORTX_STORE)
  set(USE_POSIX_STORE OFF)
  message(STATUS "Disabling POSIX Store")
endif(USE_POSIX_OBJ OR USE_CORTX_STORE)

if (USE_CORTX_STORE)
  set(RPM_REQUIRES "motr ${RPM_REQUIRES}")
  set(RPM_DEVEL_REQUIRES "motr-devel ${RPM_DEVEL_REQUIRES}")
endif (USE_CORTX_STORE)

if (USE_CORTX_STORE)
  set(EXTSTORE_OPT "CORTX")
elseif (USE_POSIX_STORE)
  set(EXTSTORE_OPT "Posix")
endif (USE_CORTX_STORE)

# Option (To enable/disable DASSERT mode.)
option(ENABLE_DASSERT "Enable ENABLE_DASSERT mode." ON)

if (ENABLE_DASSERT)
        set(BCOND_ENABLE_DASSERT "%bcond_without")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_DASSERT")
else (ENABLE_DASSERT)
        set(BCOND_ENABLE_DASSERT "%bcond_with")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif (ENABLE_DASSERT)

message( STATUS "ENABLE_DASSERT : ${ENABLE_DASSERT}")

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

# Check for CORTX
if(USE_CORTX_STORE)
  find_library(HAVE_CORTX motr)
  check_library_exists(
	motr
	m0_clovis_init
	""
	HAVE_CORTX
	)
  check_include_files("motr/config.h" HAVE_CORTX_H)

  find_library(HAVE_CORTX_HELPERS motr-helpers)
  check_library_exists(
	motr-helpers
	m0_ufid_init
	""
	HAVE_CORTX_HELPERS
	)

  if((NOT HAVE_CORTX) OR (NOT HAVE_CORTX_H) OR (NOT HAVE_CORTX_HELPERS))
      message(FATAL_ERROR "Cannot find MOTR")
  endif((NOT HAVE_CORTX) OR (NOT HAVE_CORTX_H) OR (NOT HAVE_CORTX_HELPERS))
endif(USE_CORTX_STORE)

message(STATUS "USE_POSIX_STORE=${USE_POSIX_STORE}")
message(STATUS "USE_CORTX_STORE=${USE_CORTX_STORE}")
message(STATUS "USE_POSIX_OBJ=${USE_POSIX_OBJ}")

add_subdirectory(dstore)
set(DSTORE dstore)

add_subdirectory(dsal)
set(DSAL dsal)


set(CMAKE_REQUIRED_INCLUDES ${CORTXUTILSINC})

CHECK_INCLUDE_FILES("fault.h" HAVE_CORTX_UTILS_H)

message(STATUS "HAVE_CORTX_UTILS_H=${HAVE_CORTX_UTILS_H}")

if(NOT HAVE_CORTX_UTILS_H)
 if(STRICT_PACKAGE)
    message(FATAL_ERROR "STRICT_PACKAGE: Cannot find CORTX-UTILS runtime. Disabling DSAL build")
 else(STRICT_PACKAGE)
    message(WARNING "Cannot find CORTX-UTILS runtime. Disabling DSAL build")
    set(USE_DSAL OFF)
  endif(STRICT_PACKAGE)
endif(NOT HAVE_CORTX_UTILS_H)

include_directories(${CORTXUTILSINC})
link_directories(${LIBCORTXUTILS})

add_library(${LIB_DSAL} SHARED
		$<TARGET_OBJECTS:${DSTORE}>
		$<TARGET_OBJECTS:${DSAL}>
	)

target_link_libraries(${LIB_DSAL}
  motr
  motr-helpers
  ini_config
  ${PROJECT_NAME_BASE}-utils
)

# CPack / rpmbuild specific stuff
set(CPACK_PACKAGE_FILE_NAME "${LIB_DSAL}-Source" )
set(CPACK_PACKAGE_VENDOR "DSAL")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DSAL - the data store abstraction layer")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CORTX_DSAL_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${CORTX_DSAL_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${CORTX_DSAL_PATCH_LEVEL})

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
  "${PROJECT_SOURCE_DIR}/${LIB_DSAL}.pc-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_DSAL}.pc"
)

# Now create a useable specfile
configure_file(
  "${PROJECT_SOURCE_DIR}/${LIB_DSAL}.spec-in.cmake"
  "${PROJECT_SOURCE_DIR}/${LIB_DSAL}.spec"
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

add_subdirectory(test)
