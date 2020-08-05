#!/bin/bash

MNT_PATH=/tmp/nfsmount
NUM_MNT=10
SERVER='localhost'
DLEV=5
DDIR=5
DFILE=5


die() 
{
	echo "error: $*"
	exit 1
}

mount() {

	dir_list=("$MNT_PATH"/testdir{1..10})

	[ ! -d "$MNT_PATH" ] && mkdir -p "$MNT_PATH"/testdir{1..10}

	echo -e "Mounting\n"
	m_flag=1
	for dir in ${dir_list[@]}
	do
		echo $dir
		sudo mount -t nfs4 -o 'vers=4.0' $SERVER:/ $dir
		[ $? -ne 0 ] && die "Failed to mount"
	done
}


umount() {

	dir_list=("$MNT_PATH"/testdir{1..10})
	echo -e "Unmounting\n"
	for dir in ${dir_list[@]}
	do
		echo $dir
		sudo umount -l $dir
	done
	rm -rf $MNT_PATH
}


dirtree() {

	[ -z $m_flag ] && mount

	echo "Log file written: /tmp/concurrency-$$.log"

	i=0
	for dir in ${dir_list[@]}
	do
		((i++))
		path=$dir/kvsns/
		python dirtree.py $path "Client$i" "create_dirtree" $DLEV $DDIR $DFILE &
	done

	wait
	python dirtree.py $path "Client$i" "check_dirtree" $DLEV $DDIR $DFILE

	echo "Process is complete"
}

function usage {
	cat <<EOM
usage: $0 [-h] [-n <num_mnt>] [-l <dir_lev>] [-d <num_dir>] [-f <num_file>] [--server <server>] {-m <Mount point> | -u <Mount point> | create_dir}
options:
  -h                help
  -n                Number of mount points [default: 10]
  -l                Directory level [default: 5]
  -d                Number of directories per level [default: 5]
  -f                Number of files per level [default: 5]
  -m                Mounts at specified mount point [default: '/tmp/nfsmount']
  -u                Unmounts at specified mount point [default: '/tmp/nfsmount']
  --server          Server name or IP address [default: 'localhost']
  create_dir        Create dirtree at mount points and check consistency
EOM
	exit 1
}


#main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

# read the options
OPTS=$(getopt --options hm:u:n:l:d:f: --long create_dir --long --server: --name "$0" -- "$@")

while [ ! -z $1 ]; do
	case "$1" in
		-h )            usage;;
		-m )            MNT_PATH=$2; mount;   shift 1;;
		-n )            NUM_MNT=$2;           shift 1;;
		-l )            DLEV=$2;              shift 1;;
		-d )            DDIR=$2;              shift 1;;
		-f )            DFILE=$2;             shift 1;;
		-u )            MNT_PATH=$2; umount;  exit 1;;
		--server )      SERVER=$2;            shift 1;;
		create_dir )    dirtree;;
		* )             usage;;
	esac
	shift 1
done
