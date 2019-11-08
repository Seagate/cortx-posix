#!/bin/bash
###############################################################################
###############################################################################
# Arguments

KVSFS_NFS_GANESHA_DIR=${KVSFS_NFS_GANESHA_DIR:-$PWD/../nfs-ganesha}
KVSFS_NFS_GANESHA_BUILD_DIR=${KVSFS_NFS_GANESHA_BUILD_DIR:-$KVSFS_NFS_GANESHA_DIR/build}
KVSFS_NFS_GANESHA_CMAKE_ARGS=${KVSFS_NFS_GANESHA_CMAKE_ARGS:-""}

###############################################################################
nfs_ganesha_print_env() {
    local myenv=(
    KVSFS_NFS_GANESHA_DIR
    KVSFS_NFS_GANESHA_BUILD_DIR
    KVSFS_NFS_GANESHA_CMAKE_ARGS
    )

    for i in ${myenv[@]}; do
        echo "$i=${!i}"
    done
}

###############################################################################
_nfs_ganesha_chk_pkg() {
    local pkg=$1
    yum list installed $pkg 2>&1 1>/dev/null
    local rc=$?
    if (( rc != 0 )); then
        echo "Please install dependency: 'yum install $pkg'"
    fi
    return $rc
}

###############################################################################
_nfs_ganesha_check_installed_pkgs() {
    local packages_list=(
        libini_config-devel
    )
    for pkg in ${packages_list[@]}; do
        if [ ! _nfs_ganesha_chk_pkg $pkg ]; then
            exit 1;
        fi
    done
}

###############################################################################
nfs_ganesha_bootstrap() {
    rm -fR "$KVSFS_NFS_GANESHA_DIR"
    echo "Downloading NFS Ganesha sources into $KVSFS_NFS_GANESHA_DIR"
    git clone --depth 1 ssh://git@gitlab.mero.colo.seagate.com:6022/eos/fs/nfs-ganesha.git $KVSFS_NFS_GANESHA_DIR
    cd $KVSFS_NFS_GANESHA_DIR
    git submodule update --init --recursive
    cd -
}

###############################################################################
nfs_ganesha_update() {
    if [ ! -d $KVSFS_NFS_GANESHA_DIR ]; then
        echo "Cannot find existing NFS Ganesha folder in $KVSFS_NFS_GANESHA_DIR."
        echo "Please clone it manually or use '$0 bootstrap' command."
    fi
    cd $KVSFS_NFS_GANESHA_DIR
    git submodule update --init --recursive
    cd -
}

###############################################################################
nfs_ganesha_configure() {
    local cmakerootfile="$KVSFS_NFS_GANESHA_DIR/src/CMakeLists.txt"

    if [ ! -f $cmakerootfile ]; then
        echo "Cannot find $cmakerootfile in $KVSFS_NFS_GANESHA_DIR."
        echo "Please specify another locaton or update/bootstrap the repo."
        exit 1
    fi

    if [ ! _nfs_ganesha_check_installed_pkgs ]; then
        echo "Please install dependencies and run 'config' again."
        exit 1
    fi

    echo "Configuring NFS Ganesha $KVSFS_NFS_GANESHA_DIR -> $KVSFS_NFS_GANESHA_BUILD_DIR"

    rm -fR $KVSFS_NFS_GANESHA_BUILD_DIR
    mkdir $KVSFS_NFS_GANESHA_BUILD_DIR

    cd $KVSFS_NFS_GANESHA_BUILD_DIR
    local cmake_cmd="cmake $KVSFS_NFS_GANESHA_DIR/src $KVSFS_NFS_GANESHA_CMAKE_ARGS"
    echo "CMAKE: $cmake_cmd"
    $cmake_cmd
    local rc=$?
    cd -

    echo "Finished NFS Ganesha config ($rc)"
    return $rc
}

###############################################################################
nfs_ganesha_make() {
    if [ ! -d $KVSFS_NFS_GANESHA_BUILD_DIR ]; then
        echo "Build folder $KVSFS_NFS_GANESHA_BUILD_DIR does not exist. Please run 'config'"
        exit 1;
    fi

    cd $KVSFS_NFS_GANESHA_BUILD_DIR
    make "$@"
    cd -
}

###############################################################################
nfs_ganesha_purge() {
    if [ ! -d "$KVSFS_NFS_GANESHA_BUILD_DIR" ]; then
        echo "Nothing to remove ($KVSFS_NFS_GANESHA_BUILD_DIR)"
        return 0;
    fi

    rm -fR "$KVSFS_NFS_GANESHA_BUILD_DIR"
}

###############################################################################
nfs_ganesha_usage() {
    echo -e "
NFS Ganesha Build script.
Usage:
    env <build environment> $0 <action>

Where action is one of the following:
        help    - Print usage.
        env     - Show build environment.

        config  - Clean up build dir and run configure step.

        make    - Run make [...] command.

        reinstall - Build RPMs, remove old pkgs and install new pkgs.
        rpm-gen - Build RPMs.
        rpm-install - Install RPMs build by rpm-gen.
        rpm-uninstall - Uninstall pkgs.

        kvssh   - Run KVSNS Shell from build folder.

An example of a typical workflow:
    $0 config -- Generates out-of-tree cmake build folder.
    $0 make -j -- Compiles files in it.
    $0 reinstall -- Generates and re-installs RPMs.
"
}

###############################################################################
case $1 in
    env)
        nfs_ganesha_print_env;;
    bootstrap)
        nfs_ganesha_bootstrap;;
    update)
        nfs_ganesha_update;;
    config)
        nfs_ganesha_configure;;
    purge)
        nfs_ganesha_purge;;
    make)
        shift
        nfs_ganesha_make "$@" ;;
    rpm-gen)
        nfs_ganesha_make rpm;;
    rpm-install)
        echo "Not implemented: Please install RPMs manually."
        exit 1;;
    rpm-uninstall)
        echo "Not implemented: Please uninstall RPMs manually."
        exit 1;;
    reinstall)
        echo "Not implemented: Plesae re-install local RPMs manually."
        exit 1;;
    *)
        nfs_ganesha_usage;;
esac

###############################################################################
