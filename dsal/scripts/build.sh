#!/bin/bash
###############################################################################
# Build script for DSAL component.
###############################################################################
set -e

###############################################################################
# Arguments

# Project Name.
PROJECT_NAME_BASE=${PROJECT_NAME_BASE:-"cortx"}

# Install Dir.
INSTALL_DIR_ROOT=${INSTALL_DIR_ROOT:-"/opt/seagate"}

# DSAL source repo root.
DSAL_SOURCE_ROOT=${DSAL_SOURCE_ROOT:-$PWD}

# Root folder for out-of-tree builds, i.e. location for the build folder.
# For superproject builds: it is derived from CORTX_FS_BUILD_ROOT (cortxfs/build-dsal).
# For local builds: it is based on $PWD (./build-dsal).
DSAL_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-$DSAL_SOURCE_ROOT}

# Select DSAL DSTORE implementation.
DSAL_DSTORE_BACKEND=${DSAL_DSTORE_BACKEND:-"cortx"}

# Select DSAL Source Version.
# Superproject: derived from cortxfs version.
# Local: taken fron VERSION file.
DSAL_VERSION=${CORTXFS_VERSION:-"$(cat $DSAL_SOURCE_ROOT/VERSION)"}


# Select DSAL Build Version.
# Superproject: derived from cortxfs version.
# Local: taken from git rev.
DSAL_BUILD_VERSION=${CORTXFS_BUILD_VERSION:-"$(git rev-parse --short HEAD)"}

# Optional, CORTX-UTILS source location.
# Superproject: uses pre-defined location.
# Local: searches in the top-level dir.
CORTX_UTILS_SOURCE_ROOT=${CORTX_UTILS_SOURCE_ROOT:-"$DSAL_SOURCE_ROOT/../utils/c-utils"}

# Optional, CORTX-UTILS build root location
# Superproject: derived from CORTXFS build root.
# Local: located inside cortx-utils sources.
CORTX_UTILS_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$DSAL_SOURCE_ROOT/../utils/c-utils"}

###############################################################################
# Local variables

DSAL_BUILD=$DSAL_CMAKE_BUILD_ROOT/build-dsal
DSAL_SRC=$DSAL_SOURCE_ROOT/src

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

USE_CORTX_STORE="OFF"
USE_POSIX_STORE="OFF"

case $DSAL_DSTORE_BACKEND in
    "cortx")
        USE_CORTX_STORE="ON" ;;
    "posix")
        USE_POSIX_STORE="ON" ;;
    *)
        echo "Invalid dstore configuration $DSAL_DSTORE_BACKEND"
        exit 1;;
esac

###############################################################################
dsal_print_env() {
    myenv=(
        DSAL_SOURCE_ROOT
        DSAL_CMAKE_BUILD_ROOT
        DSAL_DSTORE_BACKEND
        DSAL_VERSION
        DSAL_BUILD_VERSION
        DSAL_BUILD
        DSAL_SRC
	CORTX_UTILS_LIB
	CORTX_UTILS_INC
	USE_CORTX_STORE
	USE_POSIX_STORE
    )

    for i in ${myenv[@]}; do
        echo "$i=${!i}"
    done
}

###############################################################################
dsal_configure() {
    if [ -f $DSAL_BUILD/.config ]; then
        echo "Build folder exists. Please remove it."
        exit 1;
    fi

    mkdir $DSAL_BUILD
    cd $DSAL_BUILD

    local cmd="cmake \
-DBASE_VERSION:STRING=${DSAL_VERSION} \
-DRELEASE_VER:STRING=${DSAL_BUILD_VERSION} \
-DUSE_CORTX_STORE=${USE_CORTX_STORE} \
-DUSE_POSIX_STORE=${USE_POSIX_STORE} \
-DLIBCORTXUTILS:PATH=${CORTX_UTILS_LIB} \
-DCORTXUTILSINC:PATH=${CORTX_UTILS_INC} \
-DENABLE_DASSERT=${ENABLE_DASSERT} \
-DPROJECT_NAME_BASE:STRING=${PROJECT_NAME_BASE} \
-DINSTALL_DIR_ROOT:STRING=${INSTALL_DIR_ROOT}
$DSAL_SRC"
    echo -e "Config:\n $cmd" > $DSAL_BUILD/.config
    echo -e "Env:\n $(dsal_print_env)" >> $DSAL_BUILD/.config
    $cmd
    cd -
}

###############################################################################
dsal_make() {
    if [ ! -d $DSAL_BUILD ]; then
        echo "Build folder does not exist. Please run 'config'"
        exit 1;
    fi

    cd $DSAL_BUILD
    make "$@"
    cd -
}

###############################################################################
dsal_purge() {
    if [ ! -d "$DSAL_BUILD" ]; then
        echo "Nothing to remove"
        return 0;
    fi

    rm -fR "$DSAL_BUILD"
}

###############################################################################
dsal_jenkins_build() {
    dsal_print_env && \
        dsal_purge && \
        dsal_configure && \
        dsal_make all rpms && \
        dsal_make clean
        dsal_purge
}

###############################################################################
dsal_rpm_gen() {
    dsal_make rpms
}

dsal_rpm_install() {
    local rpms_dir=$HOME/rpmbuild/RPMS/x86_64
    local dist=$(rpm --eval '%{dist}')
    local suffix="${DSAL_VERSION}-${DSAL_BUILD_VERSION}${dist}.x86_64.rpm"
    local mypkg=(
        ${PROJECT_NAME_BASE}-dsal
        ${PROJECT_NAME_BASE}-dsal-debuginfo
        ${PROJECT_NAME_BASE}-dsal-devel
    )
    local myrpms=()

    for pkg in ${mypkg[@]}; do
        local rpm_file=$rpms_dir/$pkg-$suffix
        if [ ! -f $rpm_file ]; then
            echo "Cannot find RPM file for package "$pkg" ('$rpm_file')"
            return 1
        else
            myrpms+=( $rpm_file )
        fi
    done

    echo "Installing the following RPMs:"
    for rpm in ${myrpms[@]}; do
        echo "$rpm"
    done

    sudo yum install -y ${myrpms[@]}
    local rc=$?

    echo "Done ($rc)."
    return $rc
}

dsal_rpm_uninstall() {
    sudo yum remove -y "${PROJECT_NAME_BASE}-dsal*"
}

dsal_reinstall() {
    dsal_rpm_gen &&
        dsal_rpm_uninstall &&
        dsal_rpm_install &&
    echo "OK"
}

###############################################################################
dsal_usage() {
    echo -e "
NSAL Build script.
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

An example of a typical workflow:
    $0 config -- Generates out-of-tree cmake build folder.
    $0 make -j -- Compiles files in it.
    $0 reinstall -- Generates and re-installs RPMs.
"
}

###############################################################################
case $1 in
    env)
        dsal_print_env;;
    jenkins)
        dsal_jenkins_build;;
    config)
        dsal_configure;;
    reconf)
        dsal_purge && dsal_configure;;
    purge)
        dsal_purge;;
    make)
        shift
        dsal_make "$@" ;;
    test)
        dsal_run_tests;;
    rpm-gen)
        dsal_rpm_gen;;
    rpm-install)
        dsal_rpm_install;;
    rpm-uninstall)
        dsal_rpm_uninstall;;
    reinstall)
        dsal_reinstall;;
    *)
        dsal_usage;;
esac

###############################################################################
