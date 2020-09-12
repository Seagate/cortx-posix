# NFS Ganesha

This document provides step by step instructuctions to build NFS Ganesha and install it.

## Prerequisites
  
  Run the below mentioned command to install the necessary dependency packages.
  
    sudo yum install bison cmake flex krb5-devel krb5-libs userspace-rcu-devel libini_config libini_config-devel jemalloc-devel json-c-devel dbus-devel python-devel
    
## Building the Code
  
  Perform the below mentioned procedure to build the code of NFS Ganesha.
  
  1. Run the below mentioned command to download the NFS Ganesha repository.
  
     `./scripts/build-nfs-ganesha.sh bootstrap`
    
  2. Run the below mentioned command to initialize the build folders.
  
     `./scripts/build-nfs-ganesha.sh config`
    
  3. Run the below mentioned command to build the binaries from the sources.
  
     `./scripts/build-nfs-ganesha.sh make -j`
    
  4. Run the below mentioned command to generate RPMs.
  
    `./scripts/build-nfs-ganesha.sh rpm-gen`
    
   If the RPM generation fails, you will be prompted to install packages using the below mentioned command.
     
    `sudo yum install dbus-devel libcap-devel libblkid-devel libnfsidmap-devel libattr-devel`

## Installing the RPM (code)
  
  To install the RPMs, perform the procedure mentioned below.
  
  1. Navigate to the packages directory.
  
  2. Run the the below mentioned commands to install the RPMs.
 
    `cd ~/rpmbuild/RPMS/x86_64/`
        
    `sudo yum install ./*.rpm`

## Uninstalling the RPM (code)
  
  To uninstall the RPMs, perform the procedure mentioned below.
  
  1. Run the following command to remove NFS Ganesha.
  
    `sudo yum remove '*nfs-ganesha*`
    
  2.Run the following command to remove *libntirpc*.
  
    `sudo yum remove '*libntirpc*'`
