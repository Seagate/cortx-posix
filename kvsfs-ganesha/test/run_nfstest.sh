# Filename: run_nfstest.sh
# Description: 
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero General Public License for more details.
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com. 
#

#!/bin/bash

# Variables
IP_ADDR=$(ip route get 1 | awk '{print $NF;exit}')
MNT_PATH='/mnt'
EXPORT=kvsns
MNT_OPTS='hard,rsize=4096,wsize=4096,minorversion=0'
VERBOSE='none'
RUNTEST='all'

function die {
	echo "error: $*"
	exit 1
}

function run_nfstest {
	# Check nfstest rpm
	rpm -qi --quiet nfstest
	[ $? -ne 0 ] && die "nfstest not installed"

	#run test
	nfstest_posix --server $IP_ADDR -m $MNT_PATH -e $EXPORT -o $MNT_OPTS --nfsversion=4 --runtest $RUNTEST --verbose $VERBOSE
	[ $? -ne 0 ] && die "nfstest_posix failed"
}

function usage {
	cat <<EOM
usage: $0 [-h] [--server <server>] [-m <Mount point>] [-e <Export>] [-o <Mount Options>] [--runtest <tests>] [--verbose <Verbose>]
options:
  -h			help
  --server      Server name or IP address
  -m            Mount Point [default: '/mnt']
  -e            Exported file system to mount [default: 'kvsns']
  -o            Mount Options [default: 'hard,rsize=4096,wsize=4096,minorversion=0']
  --runtest     Comma separated list of tests to run [default: 'all']
  --verbose	    Verbose level for debug messages [default: 'none']
EOM
	exit 1
}

#main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

# read the options
OPTS=$(getopt --options hm:e:o: --long server,runtest,verbose: --name "$0" -- "$@")

while [ ! -z $1 ]; do
	case "$1" in
		-h )		usage;;
		-m )		MNT_PATH=$2; shift 1;;
		-e )		EXPORT=$2; shift 1;;
		-o )		MNT_OPTS=$2; shift 1;;
		--server )	IP_ADDR=$2; shift 1;;
		--runtest ) RUNTEST=$2; shift 1;;
		--verbose ) VERBOSE=$2; shift 1;;
		* )			usage ;;
	esac
	shift 1
done

run_nfstest
