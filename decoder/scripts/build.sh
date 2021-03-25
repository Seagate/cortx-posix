#!/bin/bash
###############################################################################
# Build script for DECODER component.

###############################################################################
set -e

###############################################################################
# Arguments

# Project Name.
PROJECT_NAME_BASE=${PROJECT_NAME_BASE:-"cortx"}

# Install Dir.
INSTALL_DIR_ROOT=${INSTALL_DIR_ROOT:-"/opt/seagate"}

DECODER_SOURCE_ROOT=${DECODER_SOURCE_ROOT:-$PWD}

# Root folder for out-of-tree builds, i.e. location for the build folder.
# For superproject builds: it is derived from CORTXFS_BUILD_ROOT (cortxfs/build-decoder).
# For local builds: it is based on $PWD (./build-decoder).
DECODER_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-$DECODER_SOURCE_ROOT}

DECODER_BUILD=$DECODER_CMAKE_BUILD_ROOT/build-decoder
DECODER_SRC=$DECODER_SOURCE_ROOT/src

DSAL_SOURCE_ROOT=${DSAL_SOURCE_ROOT:-"$DECODER_SOURCE_ROOT/../dsal"}

DSAL_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$DECODER_SOURCE_ROOT/../dsal"}

NSAL_SOURCE_ROOT=${NSAL_SOURCE_ROOT:-"$DECODER_SOURCE_ROOT/../nsal"}

NSAL_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$DECODER_SOURCE_ROOT/../nsal"}

CORTXFS_SOURCE_ROOT=${CORTXFS_SOURCE_ROOT:-"$DECODER_SOURCE_ROOT/../cortxfs"}

CORTXFS_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$DECODER_SOURCE_ROOT/../cortxfs"}

CORTX_UTILS_SOURCE_ROOT=${CORTX_UTILS_SOURCE_ROOT:-"$DECODER_SOURCE_ROOT/../utils/c-utils"}

CORTX_UTILS_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$DECODER_SOURCE_ROOT/../utils/c-utils"}

###############################################################################
# Locals

DECODER_BUILD=$DECODER_CMAKE_BUILD_ROOT/build-decoder
DECODER_SRC=$DECODER_SOURCE_ROOT/src

# Use either local header/lib or the files from libcortxfs-devel package.

if [ "x$CORTX_UTILS_SOURCE_ROOT" == "x" ]; then
CORTX_UTILS_INC="/opt/seagate/cortx/utils/c-utils"
else
CORTX_UTILS_INC="$CORTX_UTILS_SOURCE_ROOT/src/include"
fi

if [ "x$CORTX_UTILS_CMAKE_BUILD_ROOT" == "x" ]; then
CORTX_UTILS_LIB="/usr/lib64/"
else
CORTX_UTILS_LIB="$CORTX_UTILS_CMAKE_BUILD_ROOT/build-cortx-utils"
fi

if [ "x$CORTXFS_SOURCE_ROOT" == "x" ]; then
    CORTXFS_INC="/usr/include/"
else
    CORTXFS_INC="$CORTXFS_SOURCE_ROOT/src/include/"
fi

if [ "x$CORTXFS_CMAKE_BUILD_ROOT" == "x" ]; then
    CORTXFS_LIB="/usr/lib64"
else
    CORTXFS_LIB="$CORTXFS_CMAKE_BUILD_ROOT/build-cortxfs/cortxfs"
fi

if [ "x$NSAL_SOURCE_ROOT" == "x" ]; then
NSAL_INC="/usr/include/cortx/nsal"
else
NSAL_INC="$NSAL_SOURCE_ROOT/src/include"
fi

if [ "x$NSAL_CMAKE_BUILD_ROOT" == "x" ]; then
NSAL_LIB="/usr/lib64/"
else
NSAL_LIB="$NSAL_CMAKE_BUILD_ROOT/build-nsal"
fi

if [ "x$DSAL_SOURCE_ROOT" == "x" ]; then
DSAL_INC="/usr/include/cortx/dsal"
else
DSAL_INC="$DSAL_SOURCE_ROOT/src/include"
fi

if [ "x$DSAL_CMAKE_BUILD_ROOT" == "x" ]; then
DSAL_LIB="/usr/lib64/"
else
DSAL_LIB="$DSAL_CMAKE_BUILD_ROOT/build-dsal"
fi

###############################################################################
decoder_print_env() {
    myenv=(
        DECODER_SOURCE_ROOT
        DECODER_CMAKE_BUILD_ROOT
	CORTX_UTILS_LIB
	CORTX_UTILS_INC
        CORTXFS_LIB
        CORTXFS_INC
	NSAL_LIB
	NSAL_INC
	DSAL_LIB
	DSAL_INC
    )

    for i in ${myenv[@]}; do
        echo "$i=${!i}"
    done
}

###############################################################################
decoder_configure() {
    if [ -f "$DECODER_BUILD/.config" ]; then
        echo "Build folder $DECODER_BUILD has already been configured. Please remove it."
        exit 1;
    fi

    mkdir "$DECODER_BUILD"
    cd "$DECODER_BUILD"

    local cmd="cmake \
-DLIBCORTXUTILS:PATH=\"$CORTX_UTILS_LIB\" \
-DCORTXUTILSINC:PATH=\"$CORTX_UTILS_INC\" \
-DNSALINC:PATH=\"$NSAL_INC\" \
-DLIBNSAL:PATH=\"$NSAL_LIB\" \
-DDSALINC:PATH=\"$DSAL_INC\" \
-DLIBDSAL:PATH=\"$DSAL_LIB\" \
-DCORTXFSINC:PATH=\"$CORTXFS_INC\" \
-DLIBCORTXFS:PATH=\"$CORTXFS_LIB\" \
-DPROJECT_NAME_BASE:STRING=${PROJECT_NAME_BASE} \
-DINSTALL_DIR_ROOT:STRING=${INSTALL_DIR_ROOT} \
$DECODER_SRC"

    echo -e "Config:\n $cmd" > "$DECODER_BUILD/.config"
    echo -e "Env:\n $(decoder_print_env)" >> "$DECODER_BUILD/.config"
cmake \
-DLIBCORTXUTILS:PATH="$CORTX_UTILS_LIB" \
-DCORTXUTILSINC:PATH="$CORTX_UTILS_INC" \
-DNSALINC:PATH="$NSAL_INC" \
-DLIBNSAL:PATH="$NSAL_LIB" \
-DDSALINC:PATH="$DSAL_INC" \
-DLIBDSAL:PATH="$DSAL_LIB" \
-DCORTXFSINC:PATH="$CORTXFS_INC" \
-DLIBCORTXFS:PATH="$CORTXFS_LIB" \
-DPROJECT_NAME_BASE:STRING="$PROJECT_NAME_BASE" \
-DINSTALL_DIR_ROOT:STRING="$INSTALL_DIR_ROOT" \
"$DECODER_SRC"
}

###############################################################################
decoder_purge() {
    if [ ! -d "$DECODER_BUILD" ]; then
        echo "Nothing to remove"
        return 0;
    fi

    rm -fR "$DECODER_BUILD"
}

###############################################################################
decoder_make() {
    if [ ! -d "$DECODER_BUILD" ]; then
        echo "Build folder $DECODER_BUILD does not exist. Please run 'config'"
        exit 1;
    fi

    pushd "$DECODER_BUILD"
    make "$@"
    popd
}

###############################################################################
decoder_jenkins_build() {
    decoder_print_env && \
        decoder_purge && \
        decoder_configure && \
        decoder_make all rpm && \
        decoder_make clean
        decoder_purge
}

###############################################################################

decoder_usage() {
    echo -e "
DECODER Build script.
Usage:
    env <build environment> $0 <action>

Where action is one of the following:
        help    - Print usage.
        env     - Show build environment.
        jenkins - Run automated CI build.

        config  - Run configure step.
        purge   - Clean up all files generated by build/config steps.
        reconf  - Clean up build dir and run configure step.

        make    - Run make [...] command.

        reinstall - Build RPMs, remove old pkgs and install new pkgs.
        rpm-gen - Build RPMs.
        rpm-install - Install RPMs build by rpm-gen.
        rpm-uninstall - Uninstall pkgs.

        kvssh   - Run CORTXFS Shell from build folder.

An example of a typical workflow:
    $0 config -- Generates out-of-tree cmake build folder.
    $0 make -j -- Compiles files in it.
    $0 reinstall -- Generates and installs RPMs.
"
}

###############################################################################
case $1 in
    env)
        decoder_print_env;;
    jenkins)
        decoder_jenkins_build;;
    config)
        decoder_configure;;
    reconf)
        decoder_purge && decoder_configure;;
    purge)
        decoder_purge;;
    make)
        shift
        decoder_make "$@" ;;
    *)
        decoder_usage;;
esac

###############################################################################
