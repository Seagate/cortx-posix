cmake_minimum_required(VERSION 2.6.3)

set(FS_GANESHA_TEST_LIB ${PROJECT_NAME_BASE}-fs-ganesha-test)
project(${FS_GANESHA_TEST_LIB} C)

set(KVSFS_GANESHA_MAJOR_VERSION 1)
set(KVSFS_GANESHA_MINOR_VERSION 0)
set(KVSFS_GANESHA_PATCH_LEVEL 1)
set(KVSFS_GANESHA_EXTRA_VERSION ${RELEASE_VER})
set(KVSFS_GANESHA_BASE_VERSION ${BASE_VERSION})

################################################################################

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")

########### install files ###############

# rpmbuild specific stuff

set(CPACK_PACKAGE_NAME "${PROJECT_NAME_BASE}_GANESHA_TEST")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME_BASE}-ganesha" )
set(CPACK_PACKAGE_VENDOR "Seagate")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Test-Tools for ${PROJECT_NAME_BASE}-FS")
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
	"${PROJECT_SOURCE_DIR}/${FS_GANESHA_TEST_LIB}.spec-in.cmake"
	"${PROJECT_SOURCE_DIR}/${FS_GANESHA_TEST_LIB}.spec"
)

set(RPM_BUILD_OPTIONS " --define '_srcrpmdir ${CMAKE_CURRENT_BINARY_DIR}'")

add_custom_target( rpm DEPENDS dist)

add_custom_command(TARGET rpm
	COMMAND sh -c "rpmbuild ${RPM_BUILD_OPTIONS}  -tb ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz"
	VERBATIM
	DEPENDS dist)
