# Quick Start guide

This is a step by step guide to get Cortxfs ready for you on your system for file access protocol, NFS.

Before cloning, you need RHEL 7.7 machine.

Note: Following instructions are applicable only for single node setup.

## Installing Prerequisites

  ### Development Tools
  
  Run the below mentioned commands to install development tools packages.

  * `yum group list`
  * `yum group install "Development Tools"`
  
  ### Motr (using root user)
  
  Before installing Motr, below mentioned RPMs are required on the system. If rpms are not installed, refer [Cortx-Motr-Quick-Start-Guide](https://github.com/Seagate/cortx-motr/blob/main/doc/Quick-Start-Guide.rst).
  
  ```
   cortx-motr
  
   cortx-motr-devel
  
   cortx-motr-debuginfo
  ```
  
  #### Verify lnet configuration

   * Create the `/etc/modprobe.d/lnet.conf` file, if it does not exist. lnet.conf should render the output as

	`options lnet networks=tcp(eth0) config_on_load=1`
   
   * `sudo systemctl restart lnet`
   * `sudo lctl list_nids` should render the output as `ip-of-your-eth0@tcp`
   Note: Make sure that the eth0 interface is present in the node by checking ifconfig. Else, update the new interface in the file.

  #### Setup Motr
  
  Perform the below mentioned procedure to install Motr.
  
  1. Run the below mentioned command to enable Motr related systemd services.
  
     `m0singlenode activate`
    
  2. Run the below mentioned command to create the storage for Motr.
  
      `m0setup -Mv -s128`
    
  3. Run the below mentioned command to start the Motr services.
  
      `m0singlenode start`
    
  4. Run the below mentioned command to check the status of Motr services.
  
      `m0singlenode status`
      
      
	```
	### Global state --------------------------------
	 State:                  loaded
	 Autostart:              disabled
	 LNet:                   active    Wed 2020-09-02 02:59:13 MDT
	 LNet Network ID:        10.230.242.169@tcp

	### Kernel --------------------------------------
	 motr-kernel             active    Wed 2020-09-02 02:59:13 MDT
	   m0ctl                  44181  0
	   m0tr                 5283132  7 m0ctl
	   galois                 22944  1 m0tr
	   lnet                  586244  3 m0tr,ksocklnd

	### Servers -------------------------------------
	 motr-server-ha          active    Wed 2020-09-02 02:59:13 MDT    PID:4536
	 motr-server-confd       active    Wed 2020-09-02 02:59:14 MDT    PID:4693
	 motr-server@mds         active    Wed 2020-09-02 02:59:14 MDT    PID:4877
	 motr-server@ios1        active    Wed 2020-09-02 02:59:15 MDT    PID:5059
	 motr-server@cas1        active    Wed 2020-09-02 02:59:15 MDT    PID:5251

	### Trace ---------------------------------------
	```

  ### NFS Ganesha

  For information on the installation of NFS Ganesha, refer [NFS Ganesha Build and Installation Guide](https://github.com/Seagate/cortx-posix/blob/main/doc/NFS_%20Ganesha_Build_and_Installation_Guide.md).
  
  ### Dependency packages for CORTX - FS
  
  * Run the below mentioned commands to install the necessary packages. 
  
    * `sudo yum install bison cmake flex krb5-devel krb5-libs userspace-rcu-devel libini_config libini_config-devel jemalloc-devel json-c-devel dbus-devel`
    
    * `sudo yum install libcmocka-devel jemalloc doxygen`
  
    * `sudo yum install cmake3 libevent-devel openssl-devel json-c-devel python-devel`
  
## Building the code

  ### Cloning CORTX - POSIX Repository
 
  Perform the procedure mentioned below.
  
  1. Become a root user.
  
     a. Set the root user password using `sudo passwd`, and enter the required password.
   
     b. Type `su -` and enter the root password to switch to the root user mode.
  
  2. Generate and copy SSH Public Key.
  
  3. Run the below mentioned commands to complete the cloning of repository.
  
      ` $ git clone git@github.com:Seagate/cortx-posix.git -b main`
    
      ` $ cd cortx-posix`     
      
  ### CORTX - FS
  
  1. Run the below mentioned command to build libevhtp.
  
      ` ./scripts/build-libevhtp.sh`
    
  2. Run the below mentioned commands to install the libevhtp rpms from the  ~/rpmbuild/RPMS/x86_64/ directory.
    
      ` cd ~/rpmbuild/RPMS/x86_64/`
    
      ` sudo yum install libevhtp-1.2.18-2.el7.x86_64.rpm libevhtp-devel-1.2.18-2.el7.x86_64.rpm`
    
  3. Run the below mentioned command to download the sources for CORTX - FS component.
  
      `./scripts/build.sh bootstrap`
      
  4. Run the below mentioned command to initialize the build folders. To enable ADDB in CORTXFS, turn on ENABLE_TSDB_ADDB flag in build script "scripts/build.sh", or pass '-t ON' option in jenkins build.
  
      `./scripts/build.sh config`
      
  5. Run the below mentioned command to build binaries from the sources.
  
      `./scripts/build.sh make -j`
      
  6. Run the below mentioned command to generate RPMs.
  
      `./scripts/build.sh rpm-gen`
  
  ### NFS Ganesha
  
   For information on building NFS Ganesha, refer [NFS Ganesha Build and Installation Guide](https://github.com/Seagate/cortx-posix/blob/main/doc/NFS_%20Ganesha_Build_and_Installation_Guide.md).
    
## Installing
  
  ### CORTX - FS RPMs
 
  * Run the following command to install the rpms.
  
    `sudo ./scripts/build.sh rpm-install`
  
  ### NFS Ganesha RPMs
  
  For information on installing NFS Ganesha, refer [NFS Ganesha Build and Installation Guide](https://github.com/Seagate/cortx-posix/blob/main/doc/NFS_%20Ganesha_Build_and_Installation_Guide.md).
  
## Uninstalling
  
  ### CORTX - FS RPMs
  
  * Run the following command to remove the rpms.
  
     `sudo ./scripts/build.sh rpm-uninstall`
  
  ### NFS Ganesha RPMs

  For information on uninstalling NFS Ganesha, refer [NFS Ganesha Build and Installation Guide](https://github.com/Seagate/cortx-posix/blob/main/doc/NFS_%20Ganesha_Build_and_Installation_Guide.md).
  
## Reinstalling
  
  ### CORTX - FS rpms
  
  * Run the below mentioned command to reinstall the rpms.
  
      `sudo ./scripts/build.sh reinstall`

## Running the tests

For information on tests to be performed, refer [Test Instructions](https://github.com/Seagate/cortx-posix/blob/main/doc/Test_Instructions.md).

## Troubleshooting

If any error occurs, refer the [Troubleshooting Guide](https://github.com/Seagate/cortx-posix/blob/main/doc/Troubleshooting_Guide.md).

