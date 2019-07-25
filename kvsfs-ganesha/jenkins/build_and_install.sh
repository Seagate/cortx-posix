#!/bin/bash

# Re-builds re-installs KVSFS&KVSNS RPM packages on the local machine

SCRIPT_DIR=$(dirname $0)
REPO_DIR=$(realpath $SCRIPT_DIR/..)
ROOT_DIR=$(realpath $REPO_DIR/..)

DEFAULT_GANESHA_SRC=$ROOT_DIR/nfs-ganesha/src
GANESHA_SRC=${GANESHA_SRC:-$DEFAULT_GANESHA_SRC}


echo -n "Removing old packages from rpm build dir ..."
rm -fR $HOME/rpmbuild/RPMS/x86_64/lib{kvsns,fsalkvsfs}*
echo "done."

echo -n "Building RPM packages ..."
if cd $REPO_DIR && ./jenkins/build.sh -p $GANESHA_SRC 1>/tmp/pkg-update 2>&1; then
    cd $SCRIPT_DIR
    echo "done."
    echo -n "Installing new packages ..."
    sudo yum remove -y 'libkvsns*' 'libfsalkvsfs*' 1>/tmp/pkg-update 2>&1 && \
        sudo yum install -y $HOME/rpmbuild/RPMS/x86_64/lib{kvsns,fsalkvsfs}* 1>/tmp/pkg-update 2>&1 && \
        sudo mv /etc/kvsns.d/kvsns.ini.rpmsave /etc/kvsns.d/kvsns.ini
    echo "done."
fi

