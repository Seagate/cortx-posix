# EOS-FS
Support for different file access protocols (like SAMBA, NFS etc.) to Seagate EOS. Currently we only support NFS Ganesha (Userspace NFS).

### Prerequisite

Install mero:
- Latest Mero rpms (`mero` and `mero-devel`) should be installed. Take the latest rpm from this [page](http://jenkins.mero.colo.seagate.com/share/bigstorage/releases/hermi/last_successful/mero/repo/)
- `m0singlenode` service should be up and running before running nfs ganesha with mero/clovis
* Clovis sample apps should be built and it's rc files should be present. Please refer to this [page](https://github.com/seagate-ssg/clovis-sample-apps) for instructions on clovis-sample-apps.
NOTE/TODO: this requirement is here only because we don't have a script to generate kvsns.ini from a mero confxc file, and therefore we don't have a way to check if clvois is up and running.
Using clovis-sample-apps is a workaround. Eventually, KVSNS will be able to generate its config file from a mero confxc file.

Install NFS Ganesha:
* Install jemalloc (`yum install jemalloc`).
* Install jemalloc-devel (`yum install jemalloc-devel`)
* Install libntirpc-1.7.4-0.1.el7.x86_64.rpm (`rpm -ivh libntirpc-1.7.4-0.1.el7.x86_64.rpm`)
* 
* Get nfs-ganesha from the official [repo](https://github.com/nfs-ganesha/nfs-ganesha/).
* Checkout the stable branch `V2.7-stable`.Build and install the `nfs-ganesha`. 
* 
* If nfs-ganesha rpm is available directly install it. (`yum localinstall nfs-ganesha-vfs-2.7.6-0.1.el7.x86_64.rpm`)

* Make sure `jemalloc` is used as the allocator (check `make edit_cache` in the build dir). Find the directions to compile [here](https://github.com/nfs-ganesha/nfs-ganesha/wiki/Compiling).

### Build
For the following procedure *eos-fs* repository is assumed to be cloned at the path ` ~/eos-fs`

- Go to `~/eos-fs`

```sh
 $ cd ~/eos-fs
```
Note: An error of ini_config.h may occur
- Install libini_config-devel  (`yum install libini_config-devel`) 

- rpm -ql krb5-devel | grep gssapi.h

- Run `build.sh` script to generate RPMs for **kvsns** and **fsalkvsfs**

```sh
$ sudo ./jenkins/build.sh -h
usage: build.sh [-p <ganesha src path>] [-g <git version>] [-v <eos-fs version>] [-k <KV Store (mero|redis)>] [-e <ExtStore (mero|posix)>]
```
**NOTE**: `-p` is a manadatory option which points to nfs ganesha source path. `-k` and `-e` i.e key value store and extstore if not specified will assume **mero** as KV store and extstore.

Sample output of the command:

```sh
$ sudo ./jenkins/build.sh -p ~/nfs-ganesha/src
Using [VERSION=1.0.1] ...
Using [GIT_VER=f471744] ...
-- cmake version 2.8
-- libkvsns version 1.0.1
-- Disabling REDIS
-- Disabling POSIX Store
...
Wrote: /root/rpmbuild/RPMS/x86_64/libkvsns-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libkvsns-devel-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libkvsns-utils-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libkvsns-debuginfo-1.0.1-f471744.el7.x86_64.rpm
...
Wrote: /root/rpmbuild/RPMS/x86_64/libfsalkvsfs-1.0.1-f471744.el7.x86_64.rpm
Wrote: /root/rpmbuild/RPMS/x86_64/libfsalkvsfs-debuginfo-1.0.1-f471744.el7.x86_64.rpm
Executing(%clean): /bin/sh -e /var/tmp/rpm-tmp.ADZiJL
...
Built target rpm
```

### Install

Install kvsns and kvfsfs RPMS:

```sh
sudo yum install $HOME/rpmbuild/RPMS/*/lib{kvsns,fsalkvsfs}*
```

A hint: for periodic local updates you can use `jenkins/build_and_install.sh`.

### Configure
- Edit `/etc/kvsns.d/kvsns.ini`

```
[mero]
local_addr = 172.16.2.132@tcp:12345:44:301
ha_addr = 172.16.2.132@tcp:12345:45:1
profile = <0x7000000000000001:1>
proc_fid = <0x7200000000000000:0>
index_dir = /tmp
kvs_fid = <0x780000000000000b:1>
```
- Update `local_addr`, `ha_addr`, `profile` and `proc_fid` in `/etc/kvsns.d/kvsns.ini` using **clovis-sample-apps** configuration. Use values for `index_dir` and `kvs_fid` as given above. Following commands will give you the values to be filled in

```sh
$ cd /path/to/clovis-sample-apps/
$ cat .c0cprc/*

#local address
172.16.2.132@tcp:12345:44:301

#ha address
172.16.2.132@tcp:12345:45:1

#profile id
<0x7000000000000001:0>

#process id
<0x7200000000000000:0>
```
- Create the Index use by KVSNS and listed in kvsns.ini as `kvs_fid`

```sh
$ sudo m0clovis --help
Usage: m0clovis options...

where valid options are

	  -?           : display this help and exit
	  -i           : more verbose help
	  -l     string: Local endpoint address
	  -h     string: HA address
	  -f     string: Process FID
	  -p     string: Profile options for Clovis
```
Use values for the options from `/etc/kvsns.d/kvsns.ini`. For the sample configuration given above, this is how command should look like

```sh
$ m0clovis -l 172.16.2.132@tcp:12345:44:301 \
		-h 172.16.2.132@tcp:12345:45:1 \
		-p '0x7000000000000001:1' \
		-f '0x7200000000000000:0'\
		index create "<0x780000000000000b:1>"
```
- Initialize KVSNS namespace

```sh
$ cd /tmp/kvsns_build/kvsns_shell
$ ./kvsns_init
```
- Test the namespace. Use links from `/tmp/kvsns_build/kvsns_shell` to manipulate the namespace

#### Configure NFS-Ganesha
- Assuming you have built NFS-Ganesha from source. Edit `/etc/ganesha/ganesha.conf`

```
# An example of KVSFS NFS Export
EXPORT
{
	Export_Id = 77;
	Path = /;
	Pseudo = /kvsns;
	FSAL {
		Name  = KVSFS;
		kvsns_config = /etc/kvsns.d/kvsns.ini;
	}
	SecType = sys;
	client {
		clients = *;
		Squash=no_root_squash;
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
