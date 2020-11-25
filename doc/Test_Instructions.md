# Test Instructions

This document provides information on different tests and the respective procedures that must be followed in this component.

## Manual tests

  * Run the below mentioned command to create a file system.
  
    `sudo /opt/seagate/cortx/cortx-fs-ganesha/bin/nfs_setup.sh setup -d fs1`
    
  * Run the below mentioned command to create a directory, to mount the file system.
  
    `sudo mkdir /mnt/dir1`
    
  * Run the below mentioned command to mount the file system.
  
    `sudo mount -o vers=4.0 127.0.0.1:/fs1 /mnt/dir1`
    
  * Run the below mentioned command to unmount the file system.
  
    `sudo umount /mnt/dir1`
	

## Test using cortxfscli

The below mentioned code block would help you in executing the tests accurately and successfully.

```
-bash-4.2$ cortxfscli -h
usage: cortxfscli [-h] {fs,endpoint,auth} ...

CORTXFS CLI command

positional arguments:
  {fs,endpoint,auth}
    fs                create, list or delete FS.
    endpoint          create, delete and update Endpoint.
    auth              setup, show, check or remove Auth Setup.

optional arguments:
  -h, --help          show this help message and exit
```  

* Run the below mentioned command to create FS.

   `cortxfscli fs create <fs_name>`

* Run the below mentioned command to list FS.

   `cortxfscli fs list`

* Run the below mentioned command to delete FS.

   `cortxfscli fs delete <fs_name>`
   
* Run the below mentioned command to create an end point.

   `cortxfscli endpoint create <fs_name> proto=nfs,secType=sys,client=1,clients=*,Squash=root_squash,access_type=RW,protocols=3,disable_acl=true`
   
* Run the below mentioned command to delete an end point.

   `cortxfscli endpoint delete <fs_name>`

## Unit tests

  The below mentioned code block would help you in executing the unit tests accurately and successfully.
  
  ```
  $ sudo ./scripts/test.sh -h
    usage: ./scripts/test.sh [-h] [-t <Test group>]
    options:
          -h      Help
          -t      Test group to execute. Deafult: all groups are executed
    Test group list:
    nsal          NSAL unit tests
    cortxfs       CORTXFS unit tests`
  ```
  
  * Run the below mentioned command to execute all unit tests.
  
      `$ sudo ./scripts/test.sh`
  
  * Run the below mentioned command to execute specific group tests.
  
      `$ sudo ./scripts/test.sh -t cortxfs`
  
  The output is displayed below.
  
  ```
  $ sudo ./scripts/test.sh
     --
    NSAL Unit tests
    NS Tests
    Test results are logged to /var/log/cortx/test/ut/ut_nsal.log
    Total tests = 5
    Tests passed = 5
    Tests failed = 0
    ...
    CORTXFS Unit tests
    Endpoint ops Tests
    Test results are logged to /var/log/cortx/test/ut/ut_cortxfs.logs
    Total tests = 4
    Tests passed = 4
    Tests failed = 0
    ...
    IO tests
    Test results are logged to /var/log/cortx/test/ut/ut_cortxfs.log
    Total tests = 6
    Tests passed = 6
    Tests failed = 0
  ``` 
