#!/bin/bash
###############################################################################
# Build script for EFS component.
###############################################################################
set -e

###############################################################################
# Arguments

# Project Name.
PROJECT_NAME_BASE=${PROJECT_NAME_BASE:-"cortx"}

# Install Dir.
INSTALL_DIR_ROOT=${INSTALL_DIR_ROOT:-"/opt/seagate"}

# EFS source repo root.
EFS_SOURCE_ROOT=${EFS_SOURCE_ROOT:-$PWD}

# Root folder for out-of-tree builds, i.e. location for the build folder.
# For superproject builds: it is derived from EOS_FS_BUILD_ROOT (eos-fs/build-efs).
# For local builds: it is based on $PWD (./build-efs).
EFS_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-$EFS_SOURCE_ROOT}

# Select EFS Source Version.
# Superproject: derived from eos-fs version.
# Local: taken fron VERSION file.
EFS_VERSION=${EOS_FS_VERSION:-"$(cat $EFS_SOURCE_ROOT/VERSION)"}


# Select EFS Build Version.
# Superproject: derived from eos-fs version.
# Local: taken from git rev.
EFS_BUILD_VERSION=${EOS_FS_BUILD_VERSION:-"$(git rev-parse --short HEAD)"}

# Optional, EOS-UTILS source location.
# Superproject: uses pre-defined location.
# Local: searches in the top-level dir.
EOS_UTILS_SOURCE_ROOT=${EOS_UTILS_SOURCE_ROOT:-"$EFS_SOURCE_ROOT/../utils"}

# Optional, EOS-UTILS build root location
# Superproject: derived from EOS-FS build root.
# Local: located inside eos-utils sources.
EOS_UTILS_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-"$EFS_SOURCE_ROOT/../utils"}

NSAL_SOURCE_ROOT=${NSAL_SOURCE_ROOT:-"$EFS_SOURCE_ROOT/../nsal"}

NSAL_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-"$EFS_SOURCE_ROOT/../nsal"}

DSAL_SOURCE_ROOT=${DSAL_SOURCE_ROOT:-"$EFS_SOURCE_ROOT/../dsal"}

DSAL_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-"$EFS_SOURCE_ROOT/../dsal"}

###############################################################################
# Local variables

EFS_BUILD=$EFS_CMAKE_BUILD_ROOT/build-efs
EFS_SRC=$EFS_SOURCE_ROOT/src

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

if [ "x$NSAL_SOURCE_ROOT" == "x" ]; then
NSAL_INC="/usr/include/eos/nsal"
else
NSAL_INC="$NSAL_SOURCE_ROOT/src/include"
fi

if [ "x$NSAL_CMAKE_BUILD_ROOT" == "x" ]; then
NSAL_LIB="/usr/lib64/"
else
NSAL_LIB="$NSAL_CMAKE_BUILD_ROOT/build-nsal"
fi

if [ "x$DSAL_SOURCE_ROOT" == "x" ]; then
DSAL_INC="/usr/include/eos/dsal"
else
DSAL_INC="$DSAL_SOURCE_ROOT/src/include"
fi

if [ "x$DSAL_CMAKE_BUILD_ROOT" == "x" ]; then
DSAL_LIB="/usr/lib64/"
else
DSAL_LIB="$DSAL_CMAKE_BUILD_ROOT/build-dsal"
fi

###############################################################################
efs_print_env() {
    myenv=(
        EFS_SOURCE_ROOT
        EFS_CMAKE_BUILD_ROOT
        EFS_VERSION
        EFS_BUILD_VERSION
        EFS_BUILD
        EFS_SRC
	EOS_UTILS_LIB
	EOS_UTILS_INC
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
efs_configure() {
    if [ -f $EFS_BUILD/.config ]; then
        echo "Build folder exists. Please remove it."
        exit 1;
    fi

    mkdir $EFS_BUILD
    cd $EFS_BUILD

    local cmd="cmake \
-DBASE_VERSION:STRING=${EFS_VERSION} \
-DRELEASE_VER:STRING=${EFS_BUILD_VERSION} \
-DLIBEOSUTILS:PATH=${EOS_UTILS_LIB} \
-DEOSUTILSINC:PATH=${EOS_UTILS_INC} \
-DLIBNSAL:PATH=${NSAL_LIB} \
-DNSALINC:PATH=${NSAL_INC} \
-DLIBDSAL:PATH=${DSAL_LIB} \
-DDSALINC:PATH=${DSAL_INC} \
-DENABLE_DASSERT=${ENABLE_DASSERT} \
-DPROJECT_NAME_BASE:STRING=${PROJECT_NAME_BASE} \
-DINSTALL_DIR_ROOT:STRING=${INSTALL_DIR_ROOT}
$EFS_SRC"
    echo -e "Config:\n $cmd" > $EFS_BUILD/.config
    echo -e "Env:\n $(efs_print_env)" >> $EFS_BUILD/.config
    $cmd
    cd -
}

###############################################################################
efs_make() {
    if [ ! -d $EFS_BUILD ]; then
        echo "Build folder does not exist. Please run 'config'"
        exit 1;
    fi

    cd $EFS_BUILD
    make "$@"
    cd -
}

###############################################################################
efs_purge() {
    if [ ! -d "$EFS_BUILD" ]; then
        echo "Nothing to remove"
        return 0;
    fi

    rm -fR "$EFS_BUILD"
}

###############################################################################
efs_jenkins_build() {
    efs_print_env && \
        efs_purge && \
        efs_configure && \
        efs_make all rpms && \
        efs_make clean
        efs_purge
}

###############################################################################
efs_rpm_gen() {
    efs_make rpms
}

efs_rpm_install() {
    local rpms_dir=$HOME/rpmbuild/RPMS/x86_64
    local dist=$(rpm --eval '%{dist}')
    local suffix="${EFS_VERSION}-${EFS_BUILD_VERSION}${dist}.x86_64.rpm"
    local mypkg=(
        eos-efs
        eos-efs-debuginfo
        eos-efs-devel
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

efs_rpm_uninstall() {
    sudo yum remove -y 'eos-efs*'
}

efs_reinstall() {
    efs_rpm_gen &&
        efs_rpm_uninstall &&
        efs_rpm_install &&
    echo "OK"
}

###############################################################################
efs_usage() {
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
        efs_print_env;;
    jenkins)
        efs_jenkins_build;;
    config)
        efs_configure;;
    reconf)
        efs_purge && efs_configure;;
    purge)
        efs_purge;;
    make)
        shift
        efs_make "$@" ;;
    test)
        efs_run_tests;;
    rpm-gen)
        efs_rpm_gen;;
    rpm-install)
        efs_rpm_install;;
    rpm-uninstall)
       efs_rpm_uninstall;;
    reinstall)
        efs_reinstall;;
    *)
        efs_usage;;
esac

###############################################################################
