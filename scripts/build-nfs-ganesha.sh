#!/bin/bash
###############################################################################
###############################################################################
# Arguments

KVSFS_NFS_GANESHA_DIR=${KVSFS_NFS_GANESHA_DIR:-$PWD/../nfs-ganesha-eos}
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
    local stable_branch="2.8-stable-eos"
    local gan_deps=(
        bison
        cmake
        flex
        krb5-devel
        krb5-libs
        userspace-rcu-devel
    )
    local repo="ssh://git@gitlab.mero.colo.seagate.com:6022/eos/third_party/nfs-ganesha.git"
    rm -fR "$KVSFS_NFS_GANESHA_DIR"
    echo "Downloading NFS Ganesha sources into $KVSFS_NFS_GANESHA_DIR"
    git clone --depth 1 --branch $stable_branch $repo $KVSFS_NFS_GANESHA_DIR
    rc=$?
    if (( rc != 0 )); then
        echo "Failed to clone NFS Ganesha sources ($rc)"
        return $rc
    fi
    cd $KVSFS_NFS_GANESHA_DIR
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
        rpm-install [y] - Install RPMs build by rpm-gen.
        rpm-uninstall [y] - Uninstall pkgs.

        kvssh   - Run KVSNS Shell from build folder.

An example of a typical workflow:
    $0 config -- Generates out-of-tree cmake build folder.
    $0 make -j -- Compiles files in it.
    $0 reinstall -y -- Generates and re-installs RPMs automaticaly
                        (without recompiling of the source code).
"
}


###############################################################################
nfs_ganesha_rpm_gen()
{
    local rpm_dir="$HOME/rpmbuild/RPMS/x86_64"

    if [ -d $rpm_dir ]; then
        find "$rpm_dir" -name "nfs-ganesha-*" -exec rm -f "{}" ';'
        find "$rpm_dir" -name "libntirpc-*" -exec rm -f "{}" ';'
    fi

    nfs_ganesha_make rpm
}

nfs_ganesha_rpm_install()
{
    local yes="$1"
    local rpm_dir="$HOME/rpmbuild/RPMS/x86_64"

    pushd $rpm_dir &>/dev/null

    sudo yum install "$yes" ./libntirpc-*.rpm ./nfs-ganesha*.rpm

    popd &>/dev/null
}

nfs_ganesha_rpm_uninstall()
{
    local yes="$1"
    sudo yum remove $yes nfs-ganesha-* libntirpc libntirpc-devel
}

nfs_ganesha_rpm_reinstall()
{
    local yes="$1"
    nfs_ganesha_rpm_uninstall $yes
    nfs_ganesha_rpm_install $yes
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
        nfs_ganesha_rpm_gen;;
    rpm-install)
        nfs_ganesha_rpm_install "$2";;
    rpm-uninstall)
        nfs_ganesha_rpm_uninstall "$2";;
    reinstall)
        nfs_ganesha_rpm_reinstall "$2";;
    *)
        nfs_ganesha_usage;;
esac

###############################################################################
