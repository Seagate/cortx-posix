[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7e6ffd004e794ecf945f076988a9185a)](https://www.codacy.com?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Seagate/cortx-posix&amp;utm_campaign=Badge_Grade)

# CORTXFS
Support for different file access protocols (like SAMBA, NFS etc.) to Seagate CORTX. Currently we only support NFS Ganesha (Userspace NFS).

### Disclaimer
The code for file access protocol (like NFS) for CORTX is distributed across multiple repos. The code is still under active development.  We welcome anyone and everyone who is interested to join our community and track our progress and even participate in development or testing if you are so inclined!  Just please be advised that this code is not ready for production usage and is only provided to allow the external community to watch and participate in the development.

### Prerequisite

Install motr:
- Latest Motr rpms (`motr` and `motr-devel`) should be installed. Take the latest rpm from this [page](http://cortx-storage.colo.seagate.com/releases/eos/github/dev/rhel-7.7.1908/motr_last_successful/)
- `m0singlenode` service should be up and running before running nfs ganesha with motr

Install NFS Ganesha:
* Install jemalloc (`yum install jemalloc`).
* Install jemalloc-devel (`yum install jemalloc-devel`)
* Install libntirpc-1.7.4-0.1.el7.x86_64.rpm (`rpm -ivh libntirpc-1.7.4-0.1.el7.x86_64.rpm`)

* Get nfs-ganesha from the official [repo](https://github.com/nfs-ganesha/nfs-ganesha/).
* Checkout the stable branch `V2.7-stable`.Build and install the `nfs-ganesha`. 

* If nfs-ganesha rpm is available directly install it. (`yum localinstall nfs-ganesha-vfs-2.7.6-0.1.el7.x86_64.rpm`)

* Make sure `jemalloc` is used as the allocator (check `make edit_cache` in the build dir). Find the directions to compile [here](https://github.com/nfs-ganesha/nfs-ganesha/wiki/Compiling).

### Build
For the following procedure *cortxfs* repository is assumed to be cloned at the path ` ~/cortxfs`

- Go to `~/cortxfs`

```sh
 $ cd ~/cortxfs
```
Note: An error of ini_config.h may occur
- Install libini_config-devel  (`yum install libini_config-devel`) 

- rpm -ql krb5-devel | grep gssapi.h

- Run `build.sh` script to generate RPMs for **cortxfs** and **fsalkvsfs**

```sh
$ sudo ./jenkins/build.sh -h
usage: build.sh [-p <ganesha src path>] [-g <git version>] [-v <cortxfs version>] [-k <KV Store (motr|redis)>] [-e <ExtStore (motr|posix)>]
```
**NOTE**: `-p` is a manadatory option which points to nfs ganesha source path. `-k` and `-e` i.e key value store and extstore if not specified will assume **motr** as KV store and extstore.

Sample output of the command:

```sh
$ sudo ./jenkins/build.sh -p ~/nfs-ganesha/src
Using [VERSION=1.0.1] ...
Using [GIT_VER=f471744] ...
-- cmake version 2.8
-- libcortxfs version 1.0.1
-- Disabling REDIS
-- Disabling POSIX Store
...
Wrote: /root/rpmbuild/RPMS/x86_64/libcortxfs-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libcortxfs-devel-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libcortxfs-utils-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libcortxfs-debuginfo-1.0.1-f471744.el7.x86_64.rpm
...
Wrote: /root/rpmbuild/RPMS/x86_64/libfsalkvsfs-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libfsalkvsfs-debuginfo-1.0.1-f471744.el7.x86_64.rpm
Executing(%clean): /bin/sh -e /var/tmp/rpm-tmp.ADZiJL
...
Built target rpm
```

### Install

Install cortxfs and kvfsfs RPMS:

```sh
sudo yum install $HOME/rpmbuild/RPMS/*/lib{cortxfs,fsalkvsfs}*
```

A hint: for periodic local updates you can use `jenkins/build_and_install.sh`.

### Configure
- Edit `/etc/cortxfs.d/cortxfs.ini`. Create if it doesn't exist

```
[motr]
local_addr = 172.16.2.132@tcp:12345:44:301
ha_addr = 172.16.2.132@tcp:12345:45:1
profile = <0x7000000000000001:1>
proc_fid = <0x7200000000000000:0>
index_dir = /tmp
kvs_fid = <0x780000000000000b:1>
```
- Update `local_addr`, `ha_addr`, `profile` and `proc_fid` in `/etc/cortxfs.d/cortxfs.ini` using the above sample configuration file Replace the `172.16.2.132` in the cortxfs.ini by the  ip address of your local machine.
- Use values for `index_dir` and `kvs_fid` as given above.

- Create the Index use by CORTXFS and listed in cortxfs.ini as `kvs_fid`

```sh
$ sudo m0mt --help
Usage: m0mt options...

where valid options are

	  -?           : display this help and exit
	  -i           : more verbose help
	  -l     string: Local endpoint address
	  -h     string: HA address
	  -f     string: Process FID
	  -p     string: Profile options for Motr client
```
Use values for the options from `/etc/cortxfs.d/cortxfs.ini`. For the sample configuration given above, this is how command should look like

```sh
$ m0mt -l 172.16.2.132@tcp:12345:44:301 \
		-h 172.16.2.132@tcp:12345:45:1 \
		-p '0x7000000000000001:1' \
		-f '0x7200000000000000:0'\
		index create "<0x780000000000000b:1>"
```
- Initialize CORTXFS namespace

```sh
$ cd /tmp/cortxfs_build/cortxfs_shell
$ ./cortxfs_init
```
- Test the namespace. Use links from `/tmp/cortxfs_build/cortxfs_shell` to manipulate the namespace

#### Configure NFS-Ganesha
- Assuming you have built NFS-Ganesha from source. Edit `/etc/ganesha/ganesha.conf`

```
# An example of KVSFS NFS Export
EXPORT
{
	Export_Id = 77;
	Path = /;
	Pseudo = /cortxfs;
	FSAL {
		Name  = KVSFS;
		cortxfs_config = /etc/cortxfs.d/cortxfs.ini;
	}
	SecType = sys;
	client {
		clients = *;
		Squash=root_squash;
		access_type=RW;
		protocols = 4;
	}
	Filesystem_id = 192.168;
}

# KVSFS Plugin path
FSAL
{
    KVSFS
    {
        FSAL_Shared_Library = /usr/lib64/ganesha/libfsalkvsfs.so.4.2.0 ;
    }
}


NFS_Core_Param
{
    Nb_Worker = 1 ;
    Manage_Gids_Expiration = 3600;
    Plugins_Dir = /usr/lib64/ganesha/ ;
}

NFSv4
{
    # Domain Name
    DomainName = localdomain ;

    # Quick restart
    Graceless = YES;
}

```

- Start **nfs-ganesha**

```sh
$ sudo ./ganesha.nfsd -L /dev/tty -F -f /etc/ganesha/ganesha.conf
```
