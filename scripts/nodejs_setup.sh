#!/bin/bash

# Variables
NODESOURCE_DIST='https://rpm.nodesource.com/setup_10.x'

abort() {
	echo "$*"
	exit 1
}

check_prerequisites() {
	#check curl rpm
	rpm -q curl > /dev/null 2>&1
	[ $? -ne 0 ] && abort "curl RPM not installed"
}

install_nodejs() {
	rpm -q nodejs > /dev/null 2>&1
	[ $? -eq 0 ] && abort "nodejs RPM already installed"

	sudo curl -sL $NODESOURCE_DIST | sudo bash -

	sudo yum install -y nodejs

	#verify the version of nodejs
	ver=$(rpm -qi nodejs | awk -F': ' '/Version/ {print $2}')

	[[ $ver < 10 ]] && abort "node-js rpm installed is of lower version.
	                          Try using a different distribution of nodejs-source.
	                          https://github.com/nodesource/distributions/tree/master/rpm"

	#check for npm rpms
	ver=$(npm -v)
	[ $? -ne 0 ] && abort "npm module could not be installed"

	echo -e "\nnodejs and npm rpms are installed successfully.\n"
}

usage() {
	cat <<EOM
usage: $0 {install} [-h]
Commands:
   -h         Help
   install    Install all the dependency rpms for node-js server

EOM
	exit 1
}

#main

cmd=$1; shift 1

getopt --options "h" --name nodejs_setup
[ $? -ne 0 ] && usage

while [ ! -z "$1" ]; do
	case "$1" in
	-h ) usage;;
	*  ) echo -e "\nInvalid command/option \n"; usage;;
	esac
	shift 1
done

case $cmd in
	install ) check_prerequisites; install_nodejs;;
	*       ) echo -e "\nInvalid command/option \n"; usage;;
esac
