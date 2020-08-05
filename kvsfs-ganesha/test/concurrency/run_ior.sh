########################################################################################
## Filename: run_ior.sh
## Description: Multi-client IO operation consistency test tool.

## Do NOT modify or remove this copyright and confidentiality notice!
## Copyright (c) 2019, Seagate Technology, LLC.
## The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
## Portions are also trade secret. Any use, duplication, derivation, distribution
## or disclosure of this code, for any reason, not expressly authorized is
## prohibited. All other rights are expressly reserved by Seagate Technology, LLC.

## Author: Shraddha Shinde <shraddha.shinde@seagate.com>
## IOR is  used for testing performance of parallel file systems. 
## IOR uses MPI for process synchronization. 
## This script check IO operation consistency in multi-client scenario
########################################################################################

#!/bin/bash

MPIEXEC=/usr/lib64/mpich/bin/mpiexec
IOR=/usr/local/bin/ior
MNT=/tmp/ior_test
MNT_PATH=/tmp/ior_test/kvsns
API=MPIIO
SERVER=$(ip route get 1 | awk '{print $NF;exit}')
IFS=','

# Script terminates with error
function die {
	echo "Error: $1"
	exit 1
}

# Mounts directory on each client
function mount_dirs {
	[ -z $CLIENTS ] && CLIENTS=($SERVER)

	read -ra CLIENTS_ARR <<< "$CLIENTS"

	for client in ${CLIENTS_ARR[@]}; do
		$MPIEXEC -hosts=$client sudo mkdir -p $MNT
		[ $? -ne 0 ] && die "Unable to create direcory $MNT_PATH $client"
		$MPIEXEC -hosts=$client sudo mount -o vers=4.0 -t nfs4 $SERVER:/ $MNT
		[[ $? -ne 0 && $? -ne 32 ]] && die "Unable to mount $MNT_PATH on $client" 
	done
}

# Unmounts mounted directory on each client
function umount_dirs {
        for client in ${CLIENTS_ARR[@]}; do
                $MPIEXEC -hosts=$client sudo umount -l $MNT
                [ $? -ne 0 ] && echo "Unable to umount $MNT on $client"
        done
}

# Runs IOR tests on multiple clients
function run_ior_test {
	let "w=128"
	let "s=1024*1024"
	let "i=2"
	let "tid=1"
	XFERS="1048576 262144 32768 4096 1024"
	for xfer in `echo $XFERS`
	do
		let "n=8"
		until [ "$n" -gt 8 ]
		do
			let "m=$n/4"
			runid="p$n.$xfer.$API"
			date
			BLOCKS="1 10 1 10 1 10"
			for blocks in `echo $BLOCKS`
			do
				let "block=${xfer} * ${blocks}"

				#fileperproc tests
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -w -z                  -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r -z                  -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r     -C              -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r -z     -Q $m        -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r     -C -Q $m        -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r     -Z -Q $m        -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r -z     -Q $m        -F   -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r -z  -Z -Q $m -X  13 -FI  -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r -z  -Z -Q $m -X -13 -F   -o $MNT_PATH/testwrite.${runid}     -i${i} -m -t ${xfer} -b ${block}  -d 0.1

				#shared tests
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -w -z                       -o $MNT_PATH/testwrite.${runid}     -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -w                          -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -r -z                       -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1

				#checkRead
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -R                          -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1

				#checkWrite, Ignored as give offset error which is due to IOR
				#$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -W                         -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1

				#read and write
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -w -r -c                    -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -w -r -c  -C                -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1
				$MPIEXEC  -hosts=$CLIENTS $IOR -A ${tid} -a $API -w -r -c  -Z                -o $MNT_PATH/testwrite.${runid} -k  -i${i} -m -t ${xfer} -b ${block}  -d 0.1

				let "tid=$tid + 17"
			done #blocks
			date
			let "n = $n * 2"
		done #n
	done #xfer
}

function run_ior {
	##Mount directories on client
	mount_dirs

	##Run IOR tests
	run_ior_test >/tmp/ior_result.$(date +'%m.%d.%Y-%H_%M').log  2>&1

	##Unmount directories on clients
	umount_dirs
}

# Usage
function usage {
	cat <<EOM
usage: $0 [-h] [-m <mount path>] [--hosts <Clients>]
	-h              help
	-m              Mount Point                                                                         [default: '/mnt/ior_test']
	--hosts         Client list seperated by ','.Passwordless ssh required for clients from server.     [default: server] 
Log file is genrated in /tmp as ior_result.date.time.log
EOM
	exit 1
}

# read the options
OPTS=$(getopt --options hm: --long hosts: --name "$0" -- "$@")
while [ ! -z $1 ]; do
        case "$1" in
                -h )            usage;;
                -m )            MNT=$2; shift 1;;
                --hosts )       CLIENTS=$2; shift 1;;
                * )             usage ;;
        esac
        shift 1
done

run_ior
