#!/bin/bash

SCRIPT_RUNNER_DIR=$(dirname "$0")
WORKLOAD_DIR=$SCRIPT_RUNNER_DIR/filebench
WORKLOADS=$(ls $WORKLOAD_DIR)
NFILES=1000

function die {
	echo -ne "error: $*\n"
	exit 1
}

function run {
	echo -ne "\n$ $*\n"
	[ ! -z "$prompt" ] && read a &&[ "$a" = "c" -o "$a" = "C" ] && exit 1
		$*
	return $?
}

function set_values {

	if [ ! -z $workload ]; then
		sed -i '1,2d'  $workload
		sed -i "1iset \$nfiles=\\$NFILES" $workload
		sed -i "1iset \$dir=\\$MNT_DIR" $workload
	else
		for workloads in ${WORKLOADS[@]}; do
	                sed -i '1,2d'  $WORKLOAD_DIR/$workloads
		        sed -i "1iset \$nfiles=\\$NFILES" $WORKLOAD_DIR/$workloads
		 	sed -i "1iset \$dir=\\$MNT_DIR" $WORKLOAD_DIR/$workloads
		done
	fi
}

function usage {
	cat <<EOM
usage: $0 [-h] [-m <mount directory>] [-n <number of <files>] [-f all|<filename> ]
options:
	-h	help
	-m	Mounted Directory. Compulsory argument
	-n	Number of files in fileset. Default is 1000
	-f	Workload. Default will run all workloads specified in filebench directory
			io_workload.f | ns_workload.f | dir_worload.f 	: Runs specified workload
EOM
	exit 1
}


function run_filebench {

	[[ -z $MNT_DIR ]] && die "Mount directory not provided. Provide mount directory"

	rpm -qi --quiet filebench
	[ $? -ne 0 ] && die "Please install Filebench: yum install filebench"

	set_values

	rm -rf $MNT_DIR/*

	if [ ! -z $workload ]; then
		run filebench -f $workload
	else
		for workload_n in ${WORKLOADS[@]}; do
			run filebench -f $WORKLOAD_DIR/$workload_n
			[ $? -ne 0 ] && die "Filebench test Failed"
		done
	fi

	[ $? -ne 0 ] && die "Filebench test Failed"
	echo -en "Filebench test completed successfully"
}

# Main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

getopt --options "hm:n:f:" --name run_filebench
[[ $? -ne 0 ]] && usage


while [ ! -z $1 ]; do
        case "$1" in
                -h ) usage;;
		-m ) MNT_DIR=$2; shift 1;;
		-n ) NFILES=$2; shift 1;;
                -f ) workload=$WORKLOAD_DIR/$2; shift 1;;
                 * ) usage ;;
        esac
        shift 1
done

run_filebench
