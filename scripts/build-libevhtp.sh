# Filename: build-libevhtp.sh
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
