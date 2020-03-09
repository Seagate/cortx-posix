#!/bin/bash

Package=libevhtp
Tag=1.2.18
Repo=https://github.com/criticalstack/$Package/archive/$Tag.tar.gz

# Clean the CWD.
rm -rf $Package-$Tag
rm -rf $Package-$Tag.tar.gz

# Get the stable libevhtp package.
wget $Repo -O $Package-$Tag.tar.gz

# Set the build ENV.
mkdir ~/rpmbuild
mkdir ~/rpmbuild/SOURCES
mkdir ~/rpmbuild/SPECS
cp $Package-$Tag.tar.gz ~/rpmbuild/SOURCES
cp $PWD/scripts/libevhtp.spec ~/rpmbuild/SPECS

# Build the libevhtp library.
rpmbuild -ba ~/rpmbuild/SPECS/libevhtp.spec
