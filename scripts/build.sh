#!/bin/bash
###############################################################################
# Builds script for all EOS-FS components.
#
# Dev workflow:
#   ./scripts/build.sh bootstrap -- Updates sub-modules and external repos.
#   ./scripts/build.sh config -- Generates out-of-tree cmake build folders
#   ./scripts/build.sh make -j -- Compiles files in them.
#   ./scripts/build.sh install -- Generates and installs RPMs for internal
#                                 components (i.e., does not install NFS Ganesha)
#
###############################################################################
set -e

###############################################################################
###############################################################################
# CMD Inteface

eosfs_cmd_usage() {
    echo -e "
Usage $0  [-p <ganesha src path>] [-v <version>] [-b <build>] [-k {eos|redis}] [-e {eos|posix}]

Arguments:
    -p (optional) Path to an existing NFS Ganesha repository.
    -v (optional) EOS FS Source version.
    -b (optional) EOS FS Build version.
    -k (optional) NSAL KVSTORE backend (eos/redis).
    -e (optional) DSAL DSTORE backend (eos/posix).

Examples:
    $0 -p ~/nfs-ganesha -- Builds EOS-FS with a custom NFS Ganesha
    $0 -v 1.0.1 -b 99 -- Builds packages with version 1.0.1-99_<commit>.
    $0 -k redis -- Builds EOS-FS with Redis as a KVS.
"
	exit 1;
}

eosfs_parse_cmd() {
    while getopts ":b:v:p:k:e:" o; do
        case "${o}" in
        b)
            export EOS_FS_BUILD_VERSION="${OPTARG}_$(git rev-parse --short HEAD)"
            ;;
        v)
            export EOS_FS_VERSION=${OPTARG}
            ;;
        p)
            local ganpath="${OPTARG}"
            if [ -d $ganpath ]; then
                if [ -d $ganpath/src ] && [ -d $ganpath/.git ] ; then
                    echo "Found existing NFS Ganesha repo in $ganpath."
                    export KVSFS_NFS_GANESHA_DIR="$(realpath $ganpath)"
                elif [ "$(basename $ganpath)" == "src" ] && [ -d $ganpath/../.git ]; then
                    echo "Found existing NFS Ganesha repo in $ganpath/.."
                    export KVSFS_NFS_GANESHA_DIR="$(realpath $ganpath/..)"
                else
                    if (($(find $ganpath -mindepth 1 -maxdepth 1 | wc -l) != 0 )); then
                        echo "The directory $ganpath is not empty but does not have NFS Ganesha sources."
                        echo "Please either specify a correct path to NFS Ganesha repository"
                        echo "  or allow the script to use the default path."
                        exit 1
                    else
                        echo "The directory $ganpath is empty. NFS Ganesha will be downloaded into it."
                        export KVSFS_NFS_GANESHA_DIR="$(realpath $ganpath)/nfs-ganesha"
                    fi
                fi
            else
                echo "The directory $ganpath does not exist."
                echo "Please either specify a correct path to NFS Ganesha repository"
                echo "  or allow the script to use the default path."
                exit 1
            fi
            echo "NFS Ganesha repo path: $KVSFS_NFS_GANESHA_DIR"

            ;;
        k)
            export NSAL_KVSTORE_BACKEND=${OPTARG}
            ;;
        e)
            export DSAL_DSTORE_BACKEND=${OPTARG}
            ;;
        *)
            eosfs_cmd_usage
            ;;
        esac
    done
}

###############################################################################
# Env

eosfs_set_env() {
    export KVSFS_SOURCE_ROOT=$PWD/kvsfs-ganesha
    export NSAL_SOURCE_ROOT=$PWD/nsal
    export DSAL_SOURCE_ROOT=$PWD/dsal
    export EOS_UTILS_SOURCE_ROOT=$PWD/utils

    export EFS_SOURCE_ROOT=$PWD/efs

    export EOS_FS_BUILD_ROOT=${EOS_FS_BUILD_ROOT:-/tmp/eos-fs}
    export EOS_FS_VERSION=${EOS_FS_VERSION:-"$(cat $PWD/VERSION)"}
    export EOS_FS_BUILD_VERSION=${EOS_FS_BUILD_VERSION:-"$(git rev-parse --short HEAD)"}

    export NSAL_KVSTORE_BACKEND=${NSAL_KVSTORE_BACKEND:-"eos"}
    export DSAL_DSTORE_BACKEND=${DSAL_DSTORE_BACKEND:-"eos"}

    export KVSFS_NFS_GANESHA_DIR=${KVSFS_NFS_GANESHA_DIR:-$PWD/../nfs-ganesha-eos}
    export KVSFS_NFS_GANESHA_BUILD_DIR=${KVSFS_NFS_GANESHA_BUILD_DIR:-$EOS_FS_BUILD_ROOT/build-nfs-ganesha}
}

eosfs_print_env() {
    eosfs_set_env
    local myenv=(
        KVSFS_SOURCE_ROOT
	NSAL_SOURCE_ROOT
	DSAL_SOURCE_ROOT
        EOS_UTILS_SOURCE_ROOT
        EOS_FS_BUILD_ROOT
        EOS_FS_BUILD_VERSION
        NSAL_KVSTORE_BACKEND
        DSAL_DSTORE_BACKEND
        KVSFS_NFS_GANESHA_DIR
        KVSFS_NFS_GANESHA_BUILD_DIR
	EFS_SOURCE_ROOT
    )
    for i in ${myenv[@]}; do
        echo "$i=${!i}"
    done
}

###############################################################################
# Builds scripts for sub-modules

_kvsfs_build() {
    echo "KVSFS_BUILD: $@"
    $KVSFS_SOURCE_ROOT/scripts/build.sh "$@"
}

_utils_build() {
    echo "UTILS_BUILD: $@"
    $EOS_UTILS_SOURCE_ROOT/scripts/build.sh "$@"
}

_nsal_build() {
    echo "NSAL_BUILD: $@"
    $NSAL_SOURCE_ROOT/scripts/build.sh "$@"
}

_dsal_build() {
    echo "DSAL_BUILD: $@"
    $DSAL_SOURCE_ROOT/scripts/build.sh "$@"
}

_efs_build() {
    echo "EFS_BUILD: $@"
    $EFS_SOURCE_ROOT/scripts/build.sh "$@"
}

_nfs_ganesha_build() {
    echo "NFS_GANESHA_BUILD: $@"
    ./scripts/build-nfs-ganesha.sh "$@"
}

###############################################################################
eosfs_bootstrap() {
    eosfs_set_env

    if [ ! -f $UTILS_SOURCE_ROOT/src/CMakeLists.txt ]; then
        git submodule update --init --recursive $UTILS_SOURCE_ROOT
    else
        echo "Skipping bootstrap for UTILS: $UTILS_SOURCE_ROOT"
    fi

    if [ ! -f $NSAL_SOURCE_ROOT/src/CMakeLists.txt ]; then
        git submodule update --init --recursive $NSAL_SOURCE_ROOT
    else
        echo "Skipping bootstrap for NSAL: $NSAL_SOURCE_ROOT"
    fi

    if [ ! -f $DSAL_SOURCE_ROOT/src/CMakeLists.txt ]; then
	git submodule update --init --recursive $DSAL_SOURCE_ROOT
    else
        echo "Skipping bootstrap for DSAL: $DSAL_SOURCE_ROOT"
    fi

    if [ ! -f $EFS_SOURCE_ROOT/src/CMakeLists.txt ]; then
        git submodule update --init --recursive $EFS_SOURCE_ROOT
    else
        echo "Skipping bootstrap for EFS: $EFS_SOURCE_ROOT"
    fi

    if [ ! -f $KVSFS_SOURCE_ROOT/src/FSAL/FSAL_KVSFS/CMakeLists.txt ]; then
        git submodule update --init --recursive $KVSFS_SOURCE_ROOT
    else
        echo "Skipping bootstrap for KVSFS: $KVSFS_SOURCE_ROOT"
    fi

    if [ ! -f $KVSFS_NFS_GANESHA_DIR/src/CMakeLists.txt ]; then
        _nfs_ganesha_build bootstrap
    else
        echo "Skipping bootstrap for NFS Ganesha: $KVSFS_NFS_GANESHA_DIR"
    fi

}

###############################################################################
eosfs_jenkins_build() {
    local rpms_dir="$HOME/rpmbuild/RPMS/x64_86"

    if ! { eosfs_parse_cmd "$@" && eosfs_set_env && eosfs_print_env; } ; then
        echo "Failed set env variables"
        exit 1
    fi

    if [ ! eosfs_bootstrap ]; then
        echo "Failed to get extra git repositories"
        exit 1
    fi

    # Remove old build root dir
    rm -fR $EOS_FS_BUILD_ROOT

    # Remove old packages
    rm -fR "$rpms_dir/"*.rpm

    mkdir -p $EOS_FS_BUILD_ROOT &&
        _nfs_ganesha_build config &&
	_utils_build reconf &&
	_utils_build make -j all &&
        _nsal_build reconf &&
        _nsal_build make -j all &&
        _dsal_build reconf &&
        _dsal_build make -j all &&
	_efs_build reconf &&
	_efs_build make -j all &&
        _kvsfs_build reconf &&
        _kvsfs_build make -j all &&
        _utils_build rpm-gen &&
        _nsal_build rpm-gen &&
        _dsal_build rpm-gen &&
	_efs_build rpm-gen &&
        _kvsfs_build rpm-gen &&
        _kvsfs_build purge &&
	_efs_build purge &&
        _nsal_build purge &&
        _dsal_build purge &&
    echo "OK"
}

###############################################################################
eosfs_configure() {
    eosfs_set_env &&
    rm -fR "EOS_FS_BUILD_ROOT"
    mkdir -p "$EOS_FS_BUILD_ROOT" &&
    _nfs_ganesha_build config &&
    _utils_build reconf &&
    _nsal_build reconf &&
    _dsal_build reconf &&
    _efs_build reconf &&
    _kvsfs_build reconf &&
    echo "OK"
}

###############################################################################
eosfs_make() {
    eosfs_set_env &&
        _utils_build make "$@" &&
        _nsal_build make "$@" &&
        _dsal_build make "$@" &&
	_efs_build make "$@" &&
        _kvsfs_build make "$@" &&
    echo "OK"
}

###############################################################################
eosfs_rpm_gen() {
    local rpms_dir="$HOME/rpmbuild/RPMS/x64_86"

    rm -fR "$rpms_dir/nfs-ganesha*"
    rm -fR "$rpms_dir/libntirpc*"
    rm -fR "$rpms_dir/eos-utils*"
    rm -fR "$rpms_dir/eos-nsal*"
    rm -fR "$rpms_dir/eos-dsal*"
    rm -fR "$rpms_dir/eos-efs*"
    rm -fR "$rpmn_dir/kvsfs-ganesha*"

    eosfs_set_env &&
        _nfs_ganesha_build rpm-gen &&
	_utils_build rpm-gen &&
        _nsal_build rpm-gen &&
        _dsal_build rpm-gen &&
	_efs_build rpm-gen &&
        _kvsfs_build rpm-gen &&
    echo "OK"
}

eosfs_rpm_install() {
    eosfs_set_env
    sudo echo "Checking sudo access"
    _utils_build rpm-install
    _nsal_build rpm-install
    _dsal_build rpm-install
    _efs_build rpm-install
    _kvsfs_build rpm-install
}

eosfs_rpm_uninstall() {
    eosfs_set_env
    sudo echo "Checking sudo access"
    _utils_build rpm-uninstall
    _nsal_build rpm-uninstall
    _dsal_build rpm-uninstall
    _efs_build rpm-uninstall
    _kvsfs_build rpm-uninstall
}

eosfs_reinstall() {
    eosfs_set_env
    sudo echo "Checking sudo access"

    _utils_build rpm-gen &&
    _nsal_build rpm-gen &&
    _dsal_build rpm-gen &&
    _efs_build rpm-gen &&
    _kvsfs_build rpm-gen &&
    _utils_build rpm-uninstall &&
    _nsal_build rpm-uninstall &&
    _dsal_build rpm-uninstall &&
    _efs_build rpm-uninstall &&
    _kvsfs_build rpm-uninstall &&
    _utils_build rpm-install &&
    _nsal_build rpm-install &&
    _dsal_build rpm-install &&
    _efs_build rpm-install &&
    _kvsfs_build rpm-install
}

###############################################################################
eosfs_usage() {
    echo -e "
EOSFS Build script.
Usage:
    env <build environment> $0 <action>

Where action is one of the following:
    env     - Show build environment.
    help    - Print usage.
    jenkins - Run CI build.

    config  - Delete old build root, run configure, and build local deps.
    purge   - Clean up all files generated by build/config steps.

    make    - Run make [...] command.

    bootstrap - Fetch recent sub-modules, check local/external build deps.
    update    - Update existing sub-modules and external sources.

    rpm-gen       - Generate RPMs.
    rpm-install   - Install the generated RPMs.
    rpm-uninstall - Uninstall the RPMs.
    reinstall     - Generate/Uninstall/Install RPMs.

    <component> <action> - Perform action on a sub-component.
     Available components: kvsfs nfs-ganesha.

Dev workflow:
    $0 bootstrap -- Download sources for EOS-FS components.
    $0 update -- (optional) Update sources.
    $0 config -- Initialize the build folders
    $0 make -j -- and build binaries from the sources.
    $0 reinstall -- Install packages.

Sub-component examples:
    $0 kvsfs make -j -- Build KVSFS only.
    $0 nfs-ganesha rpm-gen -- Generate NFS Ganesha RPMs.

External sources:
    NFS Ganesha.
    EOS-FS needs NFS Ganesha repo to build KVSFS-FSAL module.
    It also uses a generated config.h file, so that the repo
    needs to to be configured at least.
    Default location: $(dirname $PWD)/nfs-ganesha.
"
}

###############################################################################
case $1 in
    env)
        eosfs_print_env;;
    jenkins)
        shift
        eosfs_jenkins_build "$@";;
    bootstrap)
        eosfs_bootstrap;;

# Build steps
    config)
        eosfs_configure;;
    purge)
        eosfs_purge;;
    make)
        shift
        eosfs_make "$@" ;;

# Pkg mmgmt:
    rpm-gen)
        eosfs_rpm_gen;;
    rpm-install)
        eosfs_rpm_install;;
    rpm-uninstall)
        eosfs_rpm_uninstall;;
    reinstall)
        eosfs_reinstall;;

# Sub-components:
    utils)
        shift
        eosfs_set_env
        _utils_build "$@";;
    nsal)
        shift
        eosfs_set_env
        _nsal_build "$@";;
    dsal)
        shift
        eosfs_set_env
        _dsal_build "$@";;
    efs)
	shift
	eosfs_set_env
	_efs_build "$@";;
    kvsfs)
        shift
        eosfs_set_env
        _kvsfs_build "$@";;
    nfs-ganesha)
        shift
        eosfs_set_env
        _nfs_ganesha_build "$@";;
    *)
        eosfs_usage;;
esac

###############################################################################
