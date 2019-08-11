#!/bin/bash

set -e

BASE_DIR=$(realpath $(dirname $0)/..)
cd $BASE_DIR
PROG_NAME=$(basename $0)
DIST=$(realpath $BASE_DIR/dist)
CMAKE_BUILD_DIR_ROOT=${CMAKE_BUILD_DIR_TOO:-/tmp}

usage() {
	echo "usage: $PROG_NAME [-p <ganesha src path>] [-v <version>] [-b <build>] [-k {mero|redis}] [-e {mero|posix}]" 1>&2;
	echo "    -p    Path to NFS Ganesha source src dir, e.g. ~/nfs-gaensha/src" 1>&2;
	echo "    -v    EOS FS Version" 1>&2;
	echo "    -b    EOS FS Build Number" 1>&2;
	echo "    -k    Use \"mero\" or \"redis\" for Key Value Store" 1>&2;
	echo "    -e    Use \"mero\" or \"posix\" for Backend Store" 1>&2;
	exit 1;
}

while getopts ":b:v:p:k:e:" o; do
	case "${o}" in
	b)
		BUILD_VER=${OPTARG}
		;;
	v)
		VERSION=${OPTARG}
		;;
	p)
		GANESHASRC=$(realpath ${OPTARG})
		;;
	k)
		KVSTORE_OPT=${OPTARG}
		if [[ $KVSTORE_OPT != "mero" && $KVSTORE_OPT != "redis" ]]; then
			echo "Given key-value store not supported"
			exit 0;
		fi
		;;
	e)
		EXTSTORE_OPT=${OPTARG}
		if [[ $EXTSTORE_OPT != "mero" && $EXTSTORE_OPT != "posix" ]]; then
			echo "Given extstore store not supported"
			exit 0;
		fi
		;;
	*)
		usage
		;;
	esac
done

[ -z "$BUILD_VER" ] && BUILD_VER=$(git rev-parse --short HEAD) ||
    BUILD_VER="${BUILD_VER}_$(git rev-parse --short HEAD)"
[ -z "$VERSION" ] && VERSION=$(cat $BASE_DIR/VERSION)
[ -z "$GANESHASRC" ] && usage && exit 1
[ -z "$KVSTORE_OPT" ] && USE_KVS_MERO="ON"
[ -z "$EXTSTORE_OPT" ] && USE_MERO_STORE="ON"

if [ "$EXTSTORE_OPT" == "posix" ]; then
	USE_MERO_STORE="OFF"
	USE_POSIX_STORE="ON"
elif [ "$EXTSTORE_OPT" == "mero" ]; then
	USE_MERO_STORE="ON"
	USE_POSIX_STORE="OFF"
fi

if [ "$KVSTORE_OPT" == "redis" ]; then
	USE_KVS_MERO="OFF"
	USE_KVS_REDIS="ON"
elif [ "$KVSTORE_OPT" == "mero" ]; then
	USE_KVS_MERO="ON"
	USE_KVS_REDIS="OFF"
fi

echo "Using [VERSION=${VERSION}] ..."
echo "Using [BUILD_VER=${BUILD_VER}] ..."

# TODO:
# Package generation is not consitent:
# 1. We have one extra cmake build for kvsns (one here for 'all' target and the second one in the %install section of the spec file).
# 2. ksvfs package has 3 regular files instead of 1 regular file + 2 symlinks (lib.so -> lib.so.1 -> lib.so.1.0).
# Idealy, we should use the RPM CPack generator instead of a combination of the TGZ generator and our custom commands.

# Generate RPM for KVSNS
CMAKE_BUILD_DIR_KVSNS="$CMAKE_BUILD_DIR_ROOT/kvsns_build"
mkdir -p "$CMAKE_BUILD_DIR_KVSNS"
cd "$CMAKE_BUILD_DIR_KVSNS"
cmake -DUSE_KVS_MERO=${USE_KVS_MERO} -DUSE_MERO_STORE=${USE_MERO_STORE} -DUSE_KVS_REDIS=${USE_KVS_REDIS} -DUSE_POSIX_STORE=${USE_POSIX_STORE} -DBASE_VERSION:STRING=${VERSION} -DRELEASE_VER:STRING=${BUILD_VER} $BASE_DIR/src/kvsns/
make all links rpm

# Generate RPM for FSAL for KFSFS
CMAKE_BUILD_DIR_KVSFS="$CMAKE_BUILD_DIR_ROOT/kvsfs_build"
mkdir -p "$CMAKE_BUILD_DIR_KVSFS"
cd "$CMAKE_BUILD_DIR_KVSFS"
cmake -DGANESHASRC:PATH=${GANESHASRC} -DKVSNSINC:PATH=${BASE_DIR}/src/kvsns/include -DLIBKVSNS:PATH=$CMAKE_BUILD_DIR_KVSNS/kvsns -DBASE_VERSION:STRING=${VERSION} -DRELEASE_VER:STRING=${BUILD_VER} $BASE_DIR/src/nfs-ganesha/FSAL_KVSFS/
make all rpm
