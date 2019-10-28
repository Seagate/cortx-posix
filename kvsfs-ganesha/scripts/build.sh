#!/bin/bash
###############################################################################
# Build script for KVSFS-FSAL component.
###############################################################################
set -e

###############################################################################
# Arguments

KVSFS_SOURCE_ROOT=${KVSFS_SOURCE_ROOT:-$PWD}

# Root folder for out-of-tree builds, i.e. location for the build folder.
# For superproject builds: it is derived from EOS_FS_BUILD_ROOT (eos-fs/build-kvsfs).
# For local builds: it is based on $PWD (./build-kvsfs).
KVSFS_CMAKE_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-$KVSFS_SOURCE_ROOT}

# NFS Ganesha repo location.
# For superproject builds: pre-defined.
# For local builds:  look up in the upper folder.
KVSFS_NFS_GANESHA_DIR=${KVSFS_NFS_GANESHA_DIR:-"$KVSFS_SOURCE_ROOT/../../nfs-ganesha"}

# NFS Ganesha build dir location
# For superproject builds: pre-defined.
# Local builds: in the same level where target build folder located.
KVSFS_NFS_GANESHA_BUILD_DIR=${KVSFS_GANESHA_BUILD_DIR:-"$KVSFS_NFS_GANESHA_DIR/build"}

# Select KVSNS Source Version.
# Superproject: derived from eos-fs version.
# Local: taken fron VERSION file.
KVSFS_VERSION=${EOS_FS_VERSION:-"$(cat $KVSFS_SOURCE_ROOT/VERSION)"}

# Select KVSNS Build Version.
# Superproject: derived from eos-fs version.
# Local: taken from git rev.
KVSFS_BUILD_VERSION=${EOS_FS_BUILD_VERSION:-"$(git rev-parse --short HEAD)"}


# Optional, KVSNS source location.
KVSNS_SOURCE_ROOT=${KVSNS_SOURCE_ROOT:-"$KVSFS_SOURCE_ROOT/../kvsns"}

# Optional, KVSNS build location
KVSNS_CMAKE_BUILD_ROOT=${KVSNS_BUILD_ROOT:-"$KVSFS_SOURCE_ROOT/../kvsns"}

###############################################################################
# Locals

KVSFS_BUILD=$KVSFS_CMAKE_BUILD_ROOT/build-kvsfs
KVSFS_SRC=$KVSFS_SOURCE_ROOT/src/FSAL/FSAL_KVSFS

# Use either local header/lib or the files from libkvsns-devel package.

if [ "x$KVSNS_SOURCE_ROOT" == "x" ]; then
    KVSNS_INC="/usr/include/"
else
    KVSNS_INC="$KVSNS_SOURCE_ROOT/src/include/"
fi

if [ "x$KVSNS_BUILD_ROOT" == "x" ]; then
    KVSNS_LIB="/usr/lib64"
else
    KVSNS_LIB="$KVSNS_BUILD_ROOT/build-kvsns/src/kvsns"
fi


###############################################################################
kvsfs_print_env() {
    myenv=(
        KVSNS_SOURCE_ROOT
        KVSNS_CMAKE_BUILD_ROOT
        KVSFS_SOURCE_ROOT
        KVSFS_CMAKE_BUILD_ROOT
        KVSFS_VERSION
        KVSFS_BUILD_VERSION
        KVSFS_NFS_GANESHA_DIR
        KVSFS_NFS_GANESHA_BUILD_DIR
        KVSNS_LIB
        KVSNS_INC
    )

    for i in ${myenv[@]}; do
        echo "$i=${!i}"
    done
}

###############################################################################
kvsfs_configure() {
    if [ -f $KVSFS_BUILD/.config ]; then
        echo "Build folder $KVSFS_BUILD has already been configured. Please remove it."
        exit 1;
    fi

    local GANESHA_SRC="$KVSFS_NFS_GANESHA_DIR/src"
    local GANESHA_BUILD="$KVSFS_NFS_GANESHA_BUILD_DIR"

    mkdir $KVSFS_BUILD
    cd $KVSFS_BUILD

    local cmd="cmake \
-DGANESHASRC:PATH=${GANESHA_SRC} \
-DGANESHABUILD:PATH=${GANESHA_BUILD} \
-DKVSNSINC:PATH=${KVSNS_INC} \
-DLIBKVSNS:PATH=${KVSNS_LIB} \
-DBASE_VERSION:STRING=${KVSFS_VERSION} \
-DRELEASE_VER:STRING=${KVSFS_BUILD_VERSION} \
$KVSFS_SRC"

    echo -e "Config:\n $cmd" > $KVSFS_BUILD/.config
    echo -e "Env:\n $(kvsfs_print_env)" >> $KVSFS_BUILD/.config
    $cmd
    cd -
}

###############################################################################
kvsfs_purge() {
    if [ ! -d "$KVSFS_BUILD" ]; then
        echo "Nothing to remove"
        return 0;
    fi

    rm -fR "$KVSFS_BUILD"
}

###############################################################################
kvsfs_make() {
    if [ ! -d $KVSFS_BUILD ]; then
        echo "Build folder $KVSFS_BUILD does not exist. Please run 'config'"
        exit 1;
    fi

    cd $KVSFS_BUILD
    make "$@"
    cd -
}

###############################################################################
kvsfs_jenkins_build() {
    kvsfs_print_env && \
        kvsfs_purge && \
        kvsfs_configure && \
        kvsfs_make all rpm && \
        kvsfs_make clean
        kvsfs_purge
}

###############################################################################
kvsfs_local_install() {
    local log_file=/tmp/pkg-update

    echo -n "Removing old packages from rpm build dir ..."
    rm -fR $HOME/rpmbuild/RPMS/x86_64/libfsalkvsfs*
    echo "done."

    echo -n "Building RPM packages ..."
    cd $KVSFS_BUILD >/dev/null
    if ! make rpm 1>$log_file 2>&1 ; then
        echo "Failed to build RPM packages (see $log_file)."
        exit 1
    fi
    cd - >/dev/null
    echo "done."

    echo "Installing packages..."
    sudo yum remove -y 'libfsalkvsfs*' 1>$log_file 2>&1 && \
        sudo yum install -y $HOME/rpmbuild/RPMS/x86_64/libfsalkvsfs* 1>$log_file 2>&1 && \
        echo "Done."
}

kvsfs_local_uninstall() {
    local log_file=/tmp/pkg-update
    sudo echo "Installing packages..."
    sudo yum remove -y 'libfsalkvsfs*' 1>$log_file 2>&1 && \
        echo "Done."
}

###############################################################################
kvsfs_usage() {
    echo -e "
KVSFS Build script.
Usage:
    env <build environment> $0 <action>

Where action is one of the following:
        env     - Show build environment.
        config  - Run configure step.
        reconf  - Delete old conf and run configure step.
        make    - Run make [...] command.
        purge   - Clean up all files generated by build/config steps.
        jenkins - Run CI build.
        install - Build RPMs and install them locally.
        uninstall - Uninstall RPMs from the local system.
        help    - Print usage.

An example of a typical workflow:
    $0 config -- Generates out-of-tree cmake build folder
    $0 make -j -- Compiles files in it.
    $0 install -- Generates and installs RPMs.
"
}

###############################################################################
case $1 in
    env)
        kvsfs_print_env;;
    config)
        kvsfs_configure;;
    reconf)
        kvsfs_purge && kvsfs_configure;;
    make)
        shift
        kvsfs_make "$@" ;;
    rpms)
        kvsfs_make rpm;;
    purge)
        kvsfs_purge;;
    jenkins)
        kvsfs_jenkins_build;;
    install)
        kvsfs_local_install;;
    uninstall)
        kvsfs_local_uninstall;;
    *)
        kvsfs_usage;;
esac

###############################################################################
