#!/bin/bash
###############################################################################
###############################################################################
# Arguments

KVSFS_NFS_GANESHA_DIR=${KVSFS_NFS_GANESHA_DIR:-$PWD/../nfs-ganesha-cortx}
KVSFS_NFS_GANESHA_BUILD_DIR=${KVSFS_NFS_GANESHA_BUILD_DIR:-$KVSFS_NFS_GANESHA_DIR/build}

# NFS Ganesha config options
# * Disable GPFS FSAL. The FSAL requires a python package "gpfs" to be installed,
#   otherwise, NFS Ganesha RPMs cannot be built.
# * Enable RFC ACL. This flag is used to pass RFC compliance tests and
#   to perform some checks on the NFS Ganesha side.
#   See details here:
#   https://github.com/nfs-ganesha/nfs-ganesha/commit/e00e05fb871470d1b5dcc2ae317b4dbc1dd3452e
#   https://github.com/nfs-ganesha/nfs-ganesha/commit/fccc3e3fa9fcdeccaeee96ab699de0a77b02ca0e
# * Disable unused FSALS: Ceph, Gluster, RGW, Lustre.
# * Disable 9P.
# * Enable ADMIN_TOOLS, DBUS.
#
DEFAULT_CMAKE_ARGS_LIST=(
    -DUSE_FSAL_GPFS=OFF
    -DENABLE_RFC_ACL=ON
    -DUSE_FSAL_PROXY=OFF
    -DUSE_FSAL_LUSTRE=OFF
    -DUSE_FSAL_CEPH=OFF
    -DUSE_FSAL_GLUSTER=OFF
    -DUSE_FSAL_RGW=OFF
    -DUSE_RADOS_RECOV=OFF
    -DRADOS_URLS=OFF
    -DUSE_9P=OFF
    -DUSE_ADMIN_TOOLS=ON
    -DUSE_DBUS=ON
)
DEFAULT_CMAKE_ARGS="${DEFAULT_CMAKE_ARGS_LIST[@]}"

KVSFS_NFS_GANESHA_CMAKE_ARGS=${KVSFS_NFS_GANESHA_CMAKE_ARGS:-"$DEFAULT_CMAKE_ARGS"}

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
    local rc=0
    local stable_branch="2.8-stable-cortx"
    local gan_deps=(
        bison
        cmake
        flex
        krb5-devel
        krb5-libs
        userspace-rcu-devel
        python-devel
    )
    local repo="https://github.com/Seagate/nfs-ganesha.git"
    rm -fR "$KVSFS_NFS_GANESHA_DIR"
    echo "Downloading NFS Ganesha sources into $KVSFS_NFS_GANESHA_DIR"
    git clone --depth 1 --branch $stable_branch $repo "$KVSFS_NFS_GANESHA_DIR"
    rc=$?
    if (( rc != 0 )); then
        echo "Failed to clone NFS Ganesha sources ($rc)"
        return $rc
    fi
    cd "$KVSFS_NFS_GANESHA_DIR"
    git submodule update --init --recursive
    rc=$?
    if (( rc != 0 )); then
        echo "Failed to initialize NFS Ganesha submodules ($rc)"
        return $rc
    fi
    cd -

    for dep in ${gan_deps[@]}; do
        { rpm -qa "$dep" | grep $dep; } > /dev/null 2>&1
        rc=$?
        if (( rc != 0 )); then
            echo "NFS Ganesha requires \"$dep\" to be installed ($rc)"
            return $rc
        fi
    done

	#Adding double quote to prevent word splitting
	#Since we dont want to modify nfs-ganesha code, we are doing it after checkout
	sed -i 's#--version-script=${PROJECT_BINARY_DIR}/libntirpc.map#--version-script=\\"${PROJECT_BINARY_DIR}/libntirpc.map\\"#g' \
        "$KVSFS_NFS_GANESHA_DIR/src/libntirpc/src/CMakeLists.txt"
    sed -i 's#--version-script=${CMAKE_SOURCE_DIR}/MainNFSD/libganesha_nfsd.ver#--version-script=\\"${CMAKE_SOURCE_DIR}/MainNFSD/libganesha_nfsd.ver\\"#g' \
        "$KVSFS_NFS_GANESHA_DIR/src/MainNFSD/CMakeLists.txt"
}

###############################################################################
nfs_ganesha_update() {
    if [ ! -d "$KVSFS_NFS_GANESHA_DIR" ]; then
        echo "Cannot find existing NFS Ganesha folder in $KVSFS_NFS_GANESHA_DIR."
        echo "Please clone it manually or use '$0 bootstrap' command."
    fi
    cd "$KVSFS_NFS_GANESHA_DIR"
    git submodule update --init --recursive
    cd -
}

###############################################################################
nfs_ganesha_configure() {
    local cmakerootfile="$KVSFS_NFS_GANESHA_DIR/src/CMakeLists.txt"

    if [ ! -f "$cmakerootfile" ]; then
        echo "Cannot find $cmakerootfile in $KVSFS_NFS_GANESHA_DIR."
        echo "Please specify another locaton or update/bootstrap the repo."
        exit 1
    fi

    if [ ! _nfs_ganesha_check_installed_pkgs ]; then
        echo "Please install dependencies and run 'config' again."
        exit 1
    fi

    echo "Configuring NFS Ganesha $KVSFS_NFS_GANESHA_DIR -> $KVSFS_NFS_GANESHA_BUILD_DIR"

    rm -fR "$KVSFS_NFS_GANESHA_BUILD_DIR"
    mkdir "$KVSFS_NFS_GANESHA_BUILD_DIR"

    cd "$KVSFS_NFS_GANESHA_BUILD_DIR"
    #local cmake_cmd="cmake $KVSFS_NFS_GANESHA_DIR/src $KVSFS_NFS_GANESHA_CMAKE_ARGS"
    echo "CMAKE: cmake $KVSFS_NFS_GANESHA_DIR/src $KVSFS_NFS_GANESHA_CMAKE_ARGS"
    #$cmake_cmd
	cmake "$KVSFS_NFS_GANESHA_DIR/src" $KVSFS_NFS_GANESHA_CMAKE_ARGS
    local rc=$?
    cd -

    echo "Finished NFS Ganesha config ($rc)"
    return $rc
}

###############################################################################
nfs_ganesha_make() {
    if [ ! -d "$KVSFS_NFS_GANESHA_BUILD_DIR" ]; then
        echo "Build folder $KVSFS_NFS_GANESHA_BUILD_DIR does not exist. Please run 'config'"
        exit 1;
    fi

    cd "$KVSFS_NFS_GANESHA_BUILD_DIR"
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

        kvssh   - Run CORTXFS Shell from build folder.

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
