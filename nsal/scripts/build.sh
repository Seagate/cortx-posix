#!/bin/bash
###############################################################################
# Build script for NSAL component.
###############################################################################
set -e

###############################################################################
# Arguments

# NSAL source repo root.
NSAL_SOURCE_ROOT=${NSAL_SOURCE_ROOT:-$PWD}

# Root folder for out-of-tree builds, i.e. location for the build folder.
# For superproject builds: it is derived from EOS_FS_BUILD_ROOT (eos-fs/build-nsal).
# For local builds: it is based on $PWD (./build-nsal).
NSAL_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-$NSAL_SOURCE_ROOT}

# Select NSAL KVSTORE implementation.
NSAL_KVSTORE_BACKEND=${NSAL_KVSTORE_BACKEND:-"mero"}

# Select NSAL Source Version.
# Superproject: derived from eos-fs version.
# Local: taken fron VERSION file.
NSAL_VERSION=${EOS_FS_VERSION:-"$(cat $NSAL_SOURCE_ROOT/VERSION)"}


# Select NSAL Build Version.
# Superproject: derived from eos-fs version.
# Local: taken from git rev.
NSAL_BUILD_VERSION=${EOS_FS_BUILD_VERSION:-"$(git rev-parse --short HEAD)"}

# Optional, EOS-UTILS source location.
# Superproject: uses pre-defined location.
# Local: searches in the top-level dir.
EOS_UTILS_SOURCE_ROOT=${EOS_UTILS_SOURCE_ROOT:-"$NSAL_SOURCE_ROOT/../utils"}

# Optional, EOS-UTILS build root location
# Superproject: derived from EOS-FS build root.
# Local: located inside eos-utils sources.
EOS_UTILS_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-"$NSAL_SOURCE_ROOT/../utils"}

###############################################################################
# Local variables

NSAL_BUILD=$NSAL_CMAKE_BUILD_ROOT/build-nsal
NSAL_SRC=$NSAL_SOURCE_ROOT/src

USE_KVS_MERO="OFF"
USE_KVS_REDIS="OFF"
USE_MERO_STORE="OFF"
USE_POSIX_STORE="OFF"

if [ "x$EOS_UTILS_SOURCE_ROOT" == "x" ]; then
EOS_UTILS_INC="/opt/seagate/eos/utils"
else
EOS_UTILS_INC="$EOS_UTILS_SOURCE_ROOT/src/include"
fi

if [ "x$EOS_UTILS_CMAKE_BUILD_ROOT" == "x" ]; then
EOS_UTILS_LIB="/usr/lib64/"
else
EOS_UTILS_LIB="$EOS_UTILS_CMAKE_BUILD_ROOT/build-eos-utils"
fi

case $NSAL_KVSTORE_BACKEND in
    "mero")
        USE_KVS_MERO="ON" ;;
    "redis")
        USE_KVS_REDIS="ON" ;;
    *)
        echo "Invalid KVSAL configuration $NSAL_KVSTORE_BACKEND"
        exit 1;;
esac

###############################################################################
nsal_print_env() {
    myenv=(
        NSAL_SOURCE_ROOT
        NSAL_CMAKE_BUILD_ROOT
        NSAL_KVSTORE_BACKEND
        NSAL_VERSION
        NSAL_BUILD_VERSION
        NSAL_BUILD
        NSAL_SRC
        USE_KVS_MERO
        USE_KVS_REDIS
	EOS_UTILS_LIB
	EOS_UTILS_INC
	EOS_UTILS_CMAKE_BUILD_ROOT
    )

    for i in ${myenv[@]}; do
        echo "$i=${!i}"
    done
}

###############################################################################
nsal_configure() {
    if [ -f $NSAL_BUILD/.config ]; then
        echo "Build folder exists. Please remove it."
        exit 1;
    fi

    mkdir $NSAL_BUILD
    cd $NSAL_BUILD

    local cmd="cmake \
-DUSE_KVS_MERO=${USE_KVS_MERO} \
-DUSE_KVS_REDIS=${USE_KVS_REDIS} \
-DLIBEOSUTILS:PATH=${EOS_UTILS_LIB} \
-DEOSUTILSINC:PATH=${EOS_UTILS_INC} \
-DBASE_VERSION:STRING=${NSAL_VERSION} \
-DRELEASE_VER:STRING=${NSAL_BUILD_VERSION} \
-DLIBEOSUTILS:PATH=${EOS_UTILS_LIB} \
-DEOSUTILSINC:PATH=${EOS_UTILS_INC} \
$NSAL_SRC"
    echo -e "Config:\n $cmd" > $NSAL_BUILD/.config
    echo -e "Env:\n $(nsal_print_env)" >> $NSAL_BUILD/.config
    $cmd
    cd -
}

###############################################################################
nsal_make() {
    if [ ! -d $NSAL_BUILD ]; then
        echo "Build folder does not exist. Please run 'config'"
        exit 1;
    fi

    cd $NSAL_BUILD
    make "$@"
    cd -
}

###############################################################################
nsal_purge() {
    if [ ! -d "$NSAL_BUILD" ]; then
        echo "Nothing to remove"
        return 0;
    fi

    rm -fR "$NSAL_BUILD"
}

###############################################################################
nsal_jenkins_build() {
    nsal_print_env && \
        nsal_purge && \
        nsal_configure && \
        nsal_make all rpms && \
        nsal_make clean
        nsal_purge
}

###############################################################################
nsal_rpm_gen() {
    nsal_make rpms
}

nsal_rpm_install() {
    local rpms_dir=$HOME/rpmbuild/RPMS/x86_64
    local dist=$(rpm --eval '%{dist}')
    local suffix="${NSAL_VERSION}-${NSAL_BUILD_VERSION}${dist}.x86_64.rpm"
    local mypkg=(
        eos-nsal
        eos-nsal-debuginfo
        eos-nsal-devel
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

    if [ -f /etc/nsal.d/nsal.ini.rpmsave ]; then
        sudo mv /etc/nsal.d/nsal.ini.rpmsave /etc/nsal.d/nsal.ini
    fi

    echo "Done ($rc)."
    return $rc
}

nsal_rpm_uninstall() {
    sudo yum remove -y 'eos-nsal*'
}

nsal_reinstall() {
    nsal_rpm_gen &&
        nsal_rpm_uninstall &&
        nsal_rpm_install &&
    echo "OK"
}

###############################################################################
nsal_usage() {
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
        nsal_print_env;;
    jenkins)
        nsal_jenkins_build;;
    config)
        nsal_configure;;
    reconf)
        nsal_purge && nsal_configure;;
    purge)
        nsal_purge;;
    make)
        shift
        nsal_make "$@" ;;
    test)
        nsal_run_tests;;
    rpm-gen)
        nsal_rpm_gen;;
    rpm-install)
        nsal_rpm_install;;
    rpm-uninstall)
        nsal_rpm_uninstall;;
    reinstall)
        nsal_reinstall;;
    *)
        nsal_usage;;
esac

###############################################################################
