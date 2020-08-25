#!/bin/bash

# Variables
SERVER=$(ip route get 1 | awk '{print $NF;exit}')
SCRIPT_RUNNER_DIR=$(dirname "$0")
WORKLOAD_DIR=$SCRIPT_RUNNER_DIR/fio/fio_workloads/
WORKLOADS=$(ls $WORKLOAD_DIR)
FIO_PARAMS=$SCRIPT_RUNNER_DIR/fio/fio_params
MNT_PATH=/tmp/fio_test
MNT_OPTS=vers=4.0
EXPORT=cortxfs
TIME_STAMP=$(date +'%m:%d:%Y_%H:%M')
FIO_LOG=/tmp/fio_test.log
TMP_FILE=/tmp/tmp_fio_file

#Add logs 
function log {
	echo -e "$*" >> $FIO_LOG
}

#Log and exit with error
function die {
	log "error: $*"
	echo "error: $*"
	exit 1
}

function run {
	echo -ne "\n$ $*\n"
	[ ! -z "$prompt" ] && read a &&[ "$a" = "c" -o "$a" = "C" ] && exit 1
		$*
	return $?
}

#Mount mount directory
function mount_dir {
	#Validate server
	rpcinfo -p $SERVER | grep "nfs" > /dev/null 2>&1
	[ $? -ne 0 ] && die "$SERVER is not a valid NFS-Ganesha server"

	run mkdir -p $MNT_PATH	
	[ $? -ne 0 ] && die "Failed to create directory $MNT_PATH"

	run mount -o $MNT_OPTS -t nfs4 $SERVER:/ $MNT_PATH
	[[ $? -ne 0 && $? -ne 32 ]] && die "Unable to mount $MNT_PATH"
}

#Umount mounted pointed
function umount_dir {
	umount -l $MNT_PATH
	[[ $? -ne 0 && $? -ne 32 ]] && die "Unable to umount $MNT_PATH"
}

#Parse fio paramters
function parse_fio_params {
	#check if parameter file exists
	[ ! -f $FIO_PARAMS ] && die "$FIO_PARAMS file does not exist"

	#Parse parameters
        while read line
        do
                tmp_array=($line)
                IO_SIZE+=(${tmp_array[0]})
                BLOCK_SIZE+=(${tmp_array[1]})
        done < $FIO_PARAMS
}

#Run fio test
function run_fio {
	# Check fio rpm
	rpm -qi --quiet fio
	[ $? -ne 0 ] && die "Install fio rpm"
	
	#Take file and record size from file
	parse_fio_params

	#Log date to logs
	log "\n\n#TEST run on Date-$(date +'%m:%d:%Y') Time-$(date +'%H:%M')\n"

	#Mount directory
	mount_dir

	echo -e "\nFIO test output is stored in $FIO_LOG"

	# run fio
        if [ ! -z $workload ]; then
		#Check if file exists
		[ ! -f $workload ] && die "$workload file does not exist"

		fio $workload >> $TMP_FILE 2>&1
		tmp_var=$?
		echo -e "\nTest- $(head -n 1 $TMP_FILE)"
		cat $TMP_FILE >> $FIO_LOG
		[ $tmp_var -ne 0 ] && log "FIO test Failed" && echo "FIO test Failed"
		log "\n\n"
		rm -f $TMP_FILE

	else
		for workload_n in ${WORKLOADS[@]}; 
		do
			for ((t_size=0,b_size=0; b_size < ${#IO_SIZE[@]} && t_size < ${#BLOCK_SIZE[@]}; t_size++,b_size++))
			do
				io_size=${IO_SIZE[t_size]} bl_size=${BLOCK_SIZE[b_size]} path=$MNT_PATH/$EXPORT fio $WORKLOAD_DIR/$workload_n >> $TMP_FILE 2>&1
				tmp_var=$?
				echo -e "\nTest- $(head -n 1 $TMP_FILE)"
				cat $TMP_FILE >> $FIO_LOG
				[ $tmp_var -ne 0 ] && log "FIO test Failed" && echo "FIO test Failed"
				log "\n\n"
				rm -f $TMP_FILE
			done
                done
        fi

	#umount Mount point 
	umount_dir
}

function usage {
	cat <<EOM
usage: $0 [-h] [-e <Export name>] [-f <FIO parameters file>][-m <Mount Point>] [-o <Mount Options>] [-s <server>]
options:
  -h		help
  -e		Export for mounting [default: '$EXPORT']
  -f		FIO jobfile to run [default: '${WORKLOADS[*]}']
  -m		Mount directory [default: '$MNT_PATH']
  -o		Mount options separated by ',' [default: 'vers=4.0,rsize=1048576,wsize=1048576,hard']
  -p		FIO parameter file specifying total IO and block size
  -s		Server name or IP address

For $FIO_PARAMS Format is 'Total_IO_size Block_size', every new option on newline.
#k (size in Kbytes)
#m (size in Mbytes)
#g (size in Gbytes)

sample conf file
---------------
16k 4k
16m 1m
---------------

EOM
exit 1
}

#main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

# read the options
getopt --options "he:f:l:m:o:ps:" --name run_fio

while [ ! -z $1 ]; do
	case "$1" in
		-h )            usage;;
		-e )            EXPORT=$2;          shift 1;;
		-f )            workload=$2;        shift 1;;
		-l )            FIO_PARAMS=$2;      shift 1;;
		-m )            MNT_PATH=$2;        shift 1;;
		-o )            MNT_OPTS=$2;        shift 1;;
		-p )		prompt=1;;
		-s )            SERVER=$2;          shift 1;;
		* )             usage ;;
	esac
	shift 1
done

run_fio
