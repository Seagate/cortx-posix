#!/bin/bash

set -e

BASE_DIR=$(realpath $(dirname $0)/..)
cd $BASE_DIR
PROG_NAME=$(basename $0)
DIST=$(realpath $BASE_DIR/dist)

usage() {
	echo "usage: $PROG_NAME [-p <ganesha src path>] [-v <version>] [-k {mero|redis}] [-e {mero|posix}]" 1>&2;
	echo "    -p    Path to NFS Ganesha source src dir, e.g. ~/nfs-gaensha/src" 1>&2;
	echo "    -v    EOS FS Version" 1>&2;
	echo "    -k    Use \"mero\" or \"redis\" for Key Value Store" 1>&2;
	echo "    -e    Use \"mero\" or \"posix\" for Backend Store" 1>&2;
	exit 1;
}

while getopts ":g:v:p:k:e:" o; do
	case "${o}" in
	g)
		GIT_VER=${OPTARG}
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

[ -z "$GIT_VER" ] && GIT_VER=$(git rev-parse --short HEAD)
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
echo "Using [GIT_VER=${GIT_VER}] ..."

# Remove existing directory tree and create fresh one.
\rm -rf $DIST/rpmbuild
mkdir -p $DIST/rpmbuild/SOURCES

# Remove temporary directory
\rm -rf ${DIST}/eos-fs

# Generate RPM for KVSNS
mkdir -p /tmp/kvsns_build
cd /tmp/kvsns_build
cmake -DUSE_KVS_MERO=${USE_KVS_MERO} -DUSE_MERO_STORE=${USE_MERO_STORE} -DUSE_KVS_REDIS=${USE_KVS_REDIS} -DUSE_POSIX_STORE=${USE_POSIX_STORE} -DRELEASE_VER:STRING=${GIT_VER} $BASE_DIR/src/kvsns/
make all links rpm

# Generate RPM for FSAL for KFSFS
mkdir -p /tmp/fsal_build
cd /tmp/fsal_build
cmake -DGANESHASRC:PATH=${GANESHASRC} -DKVSNSINC:PATH=${BASE_DIR}/src/kvsns/include -DLIBKVSNS:PATH=/tmp/kvsns_build/kvsns -DRELEASE_VER:STRING=${GIT_VER} $BASE_DIR/src/nfs-ganesha/FSAL_KVSFS/
make all rpm
