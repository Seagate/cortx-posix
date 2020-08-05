# Filename: run_cthon.sh
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
SERVER_PATH='/kvsns'
MNT_OPTS='vers=4.0'
RUNTEST='-a'
TESTARG='-t'

function die {
	echo "error: $*"
	exit 1
}

function run_cthon {
	# Check cthon rpm
	rpm -qi --quiet cthon04
	[ $? -ne 0 ] && die "Install cthon04 rpm"

	#run test
	pushd /opt/cthon04/ > /dev/null
	source ./server  $RUNTEST $TESTARG $IP_ADDR -p $SERVER_PATH  -m $MNT_PATH -o $MNT_OPTS
	[ $? -ne 0 ] && die "Failed to run cthon"
	popd
}

function usage {
	cat <<EOM
usage: $0 [-h] [-a|-b|-g|-s|-l] [-f|-t] [--server <server>]  [-p <server_path>] [-m <Mount point>] [-o <Mount Options>]
options:
  -h                help
  -a|-b|-g|-s|-l    Test to run [default: '-a']
                      -a	run basic, general, special, and lock tests
                      -b	run basic tests only
                      -g	run general tests only
                      -s	run special tests only
                      -l	run lock tests only
  -f|-t             Test arguments- Specifies how the basic tests are to be run [default: '-t']
                      -f	a quick functionality test
                      -t	extended test mode with timings
  --server          Server name or IP address
  -p                Server path specifies a directory on the server to mount [default: '/kvsns']
  -m                Mount Point [default: '/mnt']
  -o                Mount Options [default: 'vers=4.0']
EOM
	exit 1
}

#main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

# read the options
OPTS=$(getopt --options habgslftp:m:o: --long server: --name "$0" -- "$@")

while [ ! -z $1 ]; do
	case "$1" in
		-h )            usage;;
		-m )            MNT_PATH=$2; shift 1;;
		-o )            MNT_OPTS=$2; shift 1;;
		-p )            SERVER_PATH=$2; shift 1;;
		-a|-b|-g|-s|-l)	RUNTEST=$1;;
		-f|-t)          TESTARG=$1;;
		--server )      IP_ADDR=$2; shift 1;;
		* )             usage ;;
	esac
	shift 1
done

run_cthon
