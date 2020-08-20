#!/bin/bash

# Variables
SERVER=$(ip route get 1 | awk '{print $NF;exit}')
SCRIPT_RUNNER_DIR=$(dirname "$0")
IOZONE_PARAMS=$SCRIPT_RUNNER_DIR/iozone/iozone_params
MNT_PATH=/tmp/iozone_test
MNT_OPTS=vers=4.0
EXPORT=cortxfs
TIME_STAMP=$(date +'%m:%d:%Y_%H:%M')
IOZONE_LOG=/tmp/iozone_test.log
TMP_FILE=/tmp/tmp_iozone_file

#Add logs 
function log {
	echo -e "$*" >> $IOZONE_LOG
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
	rpcinfo -p $SERVER | grep "nfs"
	[ $? -ne 0 ] die "$SERVER is not a valid NFS-Ganesha server"

	#Mount directory
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

#Take file and record size from parameter file
function parse_iozone_params {
	#Check if file exists
	[ ! -f $IOZONE_PARAMS ] && die "$IOZONE_PARAMS file does not exist"

	#Parse parameter file
	while read line
	do
		tmp_array=($line)
		FILE_SIZE+=(${tmp_array[0]})
		REC_SIZE+=(${tmp_array[1]})
	done < $IOZONE_PARAMS
}

#Run IOZone test
function run_iozone {
	# Check iozone rpm
	rpm -qi --quiet iozone
	[ $? -ne 0 ] && die "Install IOZone rpm"

	#Take file and record size from file
	parse_iozone_params

	#Log date to logs
	log "\n\n#TEST run on Date-$(date +'%m:%d:%Y') Time-$(date +'%H:%M')\n"

	#Mount directory
	mount_dir

	echo -e "\nIOZone test output is stored in $IOZONE_LOG"

	# run iozone
	for ((f_size=0,r_size=0; f_size < ${#FILE_SIZE[@]} && r_size < ${#REC_SIZE[@]}; f_size++,r_size++))
        do
		FILE=$MNT_PATH/$EXPORT/testfile$f_size.$TIME_STAMP

		echo -e "\nTest : iozone -a -s ${FILE_SIZE[f_size]} -r ${REC_SIZE[r_size]} -f $FILE"
		log "\nTEST : iozone -a -s ${FILE_SIZE[f_size]} -r ${REC_SIZE[r_size]} -f $FILE"

		run iozone -a -s ${FILE_SIZE[f_size]} -r ${REC_SIZE[r_size]} -f $FILE 2>&1 > $TMP_FILE
		tmp_var=$?
                awk '/random/,0' $TMP_FILE >> $IOZONE_LOG
		[ $tmp_var -ne 0 ] && log "IOZone test Failed" && echo "IOZone test Failed"

	done

	[ -f $TMP_FILE ] && rm -f $TMP_FILE

	umount_dir
}

function usage {
	cat <<EOM
usage: $0 [-h] [-e <Export name>] [-f <IOZone parameters file>][-m <Mount Point>] [-o <Mount Options>] [-s <server>]
options:
  -h		help
  -e		Export for mounting [default: '$EXPORT']
  -f		IOZone parameter file specifying file and record size [default: '$CONF_FILE']
  -m		Mount directory [default: '$MNT_PATH']
  -o		Mount options separated by ',' [default: 'vers=4.0,rsize=1048576,wsize=1048576,hard']
  -s		Server name or IP address

For $CONF_FILE Format is 'file_size record_size', every new option on newline.
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
getopt --options "he:f:m:o:s:" --name run_iozone

while [ ! -z $1 ]; do
	case "$1" in
		-h )            usage;;
		-e )            EXPORT=$2;          shift 1;;
		-f )            IOZONE_PARAMS=$2;   shift 1;;
		-m )            MNT_PATH=$2;        shift 1;;
		-o )            MNT_OPTS=$2;        shift 1;;
		-s )            SERVER=$2;          shift 1;;
		* )             usage ;;
	esac
	shift 1
done

run_iozone
