# NFS Ganesha

This document provides step by step instructuctions to build NFS Ganesha and install it.

## Prerequisites

  ### Install pre-requisites packages for NFS Ganesha
  
  * Run the below mentioned command to install the necessary dependency packages.
  
    * `sudo yum install bison cmake flex krb5-devel krb5-libs userspace-rcu-devel libini_config libini_config-devel jemalloc-devel json-c-devel dbus-devel python-devel`
    
## Building the Code
  
  Perform the below mentioned procedure to build the code of NFS Ganesha.
  
  1. Run the below mentioned command to download the NFS Ganesha repository.
  
    * `./scripts/build-nfs-ganesha.sh bootstrap`
    
  2. Run the below mentioned command to initialize the build folders.
  
    * `./scripts/build-nfs-ganesha.sh config`
    
  3. Run the below mentioned command to build the binaries from the sources.
  
    * `./scripts/build-nfs-ganesha.sh make -j`
    
  4. Run the below mentioned command to generate RPMs.
  
    * `./scripts/build-nfs-ganesha.sh rpm-gen`
    
      If rpm-gen failed it will prompt to install following packages
        * `sudo yum install dbus-devel libcap-devel libblkid-devel libnfsidmap-devel libattr-devel`

## Installing the rpms/code
  
  ### Install NFS Ganesha rpms
  * Go into the packages directory and install the RPMS
    * `cd ~/rpmbuild/RPMS/x86_64/`
    * `sudo yum install ./*.rpm`

## Uninstalling the rpms/code
  
  ### Uninstall NFS Ganesha rpms
  * remove nfs-ganesha
    * `sudo yum remove '*nfs-ganesha*'`
  * remove libntirpc
    * `sudo yum remove '*libntirpc*'`
