#!/bin/bash
###############################################################################
# Build script for CORTXFS component.
###############################################################################
set -e

###############################################################################
# Arguments

# Project Name.
PROJECT_NAME_BASE=${PROJECT_NAME_BASE:-"cortx"}

# Install Dir.
INSTALL_DIR_ROOT=${INSTALL_DIR_ROOT:-"/opt/seagate"}

# CORTXFS source repo root.
CORTXFS_SOURCE_ROOT=${CORTXFS_SOURCE_ROOT:-$PWD}

# Root folder for out-of-tree builds, i.e. location for the build folder.
# For superproject builds: it is derived from CORTXFS_BUILD_ROOT (cortxfs/build-cortxfs).
# For local builds: it is based on $PWD (./build-cortxfs).
CORTXFS_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-$CORTXFS_SOURCE_ROOT}

# Select CORTXFS Source Version.
# Superproject: derived from cortxfs version.
# Local: taken fron VERSION file.
CORTXFS_VERSION=${CORTXFS_VERSION:-"$(cat $CORTXFS_SOURCE_ROOT/VERSION)"}


# Select CORTXFS Build Version.
# Superproject: derived from cortxfs version.
# Local: taken from git rev.
CORTXFS_BUILD_VERSION=${CORTXFS_BUILD_VERSION:-"$(git rev-parse --short HEAD)"}

# Optional, CORTX-UTILS source location.
# Superproject: uses pre-defined location.
# Local: searches in the top-level dir.
CORTX_UTILS_SOURCE_ROOT=${CORTX_UTILS_SOURCE_ROOT:-"$CORTXFS_SOURCE_ROOT/../utils/c-utils"}

# Optional, CORTX-UTILS build root location
# Superproject: derived from CORTXFS build root.
# Local: located inside cortx-utils sources.
CORTX_UTILS_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$CORTXFS_SOURCE_ROOT/../utils/c-utils"}

NSAL_SOURCE_ROOT=${NSAL_SOURCE_ROOT:-"$CORTXFS_SOURCE_ROOT/../nsal"}

NSAL_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$CORTXFS_SOURCE_ROOT/../nsal"}

DSAL_SOURCE_ROOT=${DSAL_SOURCE_ROOT:-"$CORTXFS_SOURCE_ROOT/../dsal"}

DSAL_CMAKE_BUILD_ROOT=${CORTXFS_BUILD_ROOT:-"$CORTXFS_SOURCE_ROOT/../dsal"}

###############################################################################
# Local variables

CORTXFS_BUILD=$CORTXFS_CMAKE_BUILD_ROOT/build-cortxfs
CORTXFS_SRC=$CORTXFS_SOURCE_ROOT/src

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
cortxfs_print_env() {
    myenv=(
        CORTXFS_SOURCE_ROOT
        CORTXFS_CMAKE_BUILD_ROOT
        CORTXFS_VERSION
        CORTXFS_BUILD_VERSION
        CORTXFS_BUILD
        CORTXFS_SRC
	CORTX_UTILS_LIB
	CORTX_UTILS_INC
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
cortxfs_configure() {
    if [ -f $CORTXFS_BUILD/.config ]; then
        echo "Build folder exists. Please remove it."
        exit 1;
    fi

    mkdir $CORTXFS_BUILD
    cd $CORTXFS_BUILD

    local cmd="cmake \
-DBASE_VERSION:STRING=${CORTXFS_VERSION} \
-DRELEASE_VER:STRING=${CORTXFS_BUILD_VERSION} \
-DLIBCORTXUTILS:PATH=${CORTX_UTILS_LIB} \
-DCORTXUTILSINC:PATH=${CORTX_UTILS_INC} \
-DLIBNSAL:PATH=${NSAL_LIB} \
-DNSALINC:PATH=${NSAL_INC} \
-DLIBDSAL:PATH=${DSAL_LIB} \
-DDSALINC:PATH=${DSAL_INC} \
-DENABLE_DASSERT=${ENABLE_DASSERT} \
-DPROJECT_NAME_BASE:STRING=${PROJECT_NAME_BASE} \
-DINSTALL_DIR_ROOT:STRING=${INSTALL_DIR_ROOT}
$CORTXFS_SRC"
    echo -e "Config:\n $cmd" > $CORTXFS_BUILD/.config
    echo -e "Env:\n $(cortxfs_print_env)" >> $CORTXFS_BUILD/.config
    $cmd
    cd -
}

###############################################################################
cortxfs_make() {
    if [ ! -d $CORTXFS_BUILD ]; then
        echo "Build folder does not exist. Please run 'config'"
        exit 1;
    fi

    cd $CORTXFS_BUILD
    make "$@"
    cd -
}

###############################################################################
cortxfs_purge() {
    if [ ! -d "$CORTXFS_BUILD" ]; then
        echo "Nothing to remove"
        return 0;
    fi

    rm -fR "$CORTXFS_BUILD"
}

###############################################################################
cortxfs_jenkins_build() {
    cortxfs_print_env && \
        cortxfs_purge && \
        cortxfs_configure && \
        cortxfs_make all rpms && \
        cortxfs_make clean
        cortxfs_purge
}

###############################################################################
cortxfs_rpm_gen() {
    cortxfs_make rpms
}

cortxfs_rpm_install() {
    local rpms_dir=$HOME/rpmbuild/RPMS/x86_64
    local dist=$(rpm --eval '%{dist}')
    local suffix="${CORTXFS_VERSION}-${CORTXFS_BUILD_VERSION}${dist}.x86_64.rpm"
    local mypkg=(
        ${PROJECT_NAME_BASE}-fs
        ${PROJECT_NAME_BASE}-fs-debuginfo
        ${PROJECT_NAME_BASE}-fs-devel
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

cortxfs_rpm_uninstall() {
    sudo yum remove -y "${PROJECT_NAME_BASE}-fs*"
}

cortxfs_reinstall() {
    cortxfs_rpm_gen &&
        cortxfs_rpm_uninstall &&
        cortxfs_rpm_install &&
    echo "OK"
}

###############################################################################
cortxfs_usage() {
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
        cortxfs_print_env;;
    jenkins)
        cortxfs_jenkins_build;;
    config)
        cortxfs_configure;;
    reconf)
        cortxfs_purge && cortxfs_configure;;
    purge)
        cortxfs_purge;;
    make)
        shift
        cortxfs_make "$@" ;;
    test)
        cortxfs_run_tests;;
    rpm-gen)
        cortxfs_rpm_gen;;
    rpm-install)
        cortxfs_rpm_install;;
    rpm-uninstall)
       cortxfs_rpm_uninstall;;
    reinstall)
        cortxfs_reinstall;;
    *)
        cortxfs_usage;;
esac

###############################################################################
