# EOS-NFS
Support for NFSv4.1 to Seagate EOS

## Build
For the following procedure *eos-nfs* repository is assumed to be cloned at the path ` ~/eos-nfs`
### KVSNS
#### Build
- Create build directory
```sh
 $ mkdir /tmp/kvsns_build
 ```
- Run `cmake` from this build directory
```sh
$ cd /tmp/kvsns_build
$ cmake -DUSE_KVS_MERO=ON -D USE_MERO_STORE=ON ~/eos-nfs/src/kvsns # use appropriate path for kvsns
```
- Compile
```sh
$ make all links rpm
```

#### Install
- Install the **libkvsns** and **libkvsns-devel** rpm. 
- Use `yum install` on previously compiled RPMs
```sh
$ yum -y install /root/rpmbuild/RPMS/x86_64/libkvsns-devel-1.2.4-0.fzj.el7.x86_64.rpm
$ yum -y install /root/rpmbuild/RPMS/x86_64/libkvsns-1.2.4-0.fzj.el7.x86_64.rpm
```
**NOTE:** Path for RPMs can be different, use appropriate path generated from `$ make all links rpm`

#### Configure
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
- Update `local_addr`, `ha_addr`, `profile` and `proc_fid` using clovis-sample-apps configuration. Use values for `index_dir` and `kvs_fid` as mentioned above.
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
m0clovis -l 172.16.2.132@tcp:12345:44:301 \
         -h 172.16.2.132@tcp:12345:45:1 \
         -p '0x7000000000000001:1' \
		 -f '0x7200000000000000:0'\
		 index create "<0x780000000000000b:1>"
```
- Initialize KVSNS namespace
```sh
$ cd /tmp/kvsns_build/kvsns_shell
$ ./ns_init
```
- Test the namespace. Use links from `/tmp/kvsns_build/kvsns_shell` to manipulate the namespace
 
### FSAL-KVSFS
#### Build
- Create a build directory
```sh
$ mkdir /tmp/fsal_build
```
- Run cmake from this build directory
```sh
$ cd /tmp/fsal_build
$ sudo cmake ~/src/eos-nfs/src/FSAL_KVSFS/ -DGANESHASRC:PATH=~/nfs-ganesha/src/ -DFSALSRC:PATH=~/eos-nfs/src/kvsfs/FSAL_KVSFS
```
**NOTE:** Please use appropriate path for `GANESHASRC` *(path to source of nfs-ganesha)* and `FSALSRC` *(path to sources of FSAL)*
- Finally compile and install
```sh
$ sudo make
$ sudo make install
```

#### Configure NFS-Ganesha
- Install the nfs-ganesha rpms
- Edit `/etc/ganesha/ganesha.conf`
```
EXPORT
{
        # Export Id (mandatory, each EXPORT must have a unique Export_Id)
        Export_Id = 77;

        # Exported path (mandatory)
        Path = "/" ;

        # Pseudo Path (required for NFS v4)
        Pseudo = /kvsns;

        # Required for access (default is None)
        # Could use CLIENT blocks instead
        Access_Type = RW;

        # Exporting FSAL
        FSAL {
                Name = KVSFS;
                kvsns_config = /etc/kvsns.d/kvsns.ini;
        }
}

NFS_Core_Param
{
        Nb_Worker = 1;
}
```
- Start ganesha using `systemctl`
```sh
$ systemctl start nfs-ganesha
```
