I. BUILDING KVSNS
--------------
I make here the assumption that KVSNS sources are under ~/kvsns
1- create build directory
	# mkdir /tmp/kvsns_build
2- run cmake from this build directory
	# cd /tmp/kvsns_build
	# cmake -DUSE_KVS_MERO=ON -D USE_MERO_STORE=ON ~/kvsns
3- Compile
	# make all links rpm

II. INSTALLING KVSNS
----------------
1- Install the libkvsns and libkvsns-devel rpm
	# use "yum install" on previously compiled RPMs


III. CONFGURING KVSNS
----------------
1- edit /etc/kvsns.d/kvsns.ini  . The [mero] stanza is looking like this on client-21
   You will need to adapt it to other nodes. For the moment, kvsns uses c0cp/c0cat endpoints
	[mero]
        local_addr = 172.18.1.21@tcp:12345:41:302
        ha_addr = 172.18.1.21@tcp:12345:34:101
        profile = <0x7000000000000001:1>
        proc_fid = <0x7200000000000000:0>
        index_dir = /tmp
        kvs_fid = <0x780000000000000b:1>
2- Create the Index use by KVSNS and listed in kvsns.ini as "kvs_fid"
	# m0clovis -l 172.18.1.21@tcp:12345:41:302 \
		   -h 172.18.1.21@tcp:12345:34:101 \
                   -p '0x7000000000000001:1' \
		   -f '0x7200000000000000:0'\
		    index create "<0x780000000000000b:1>"
3- init KVSNS namespace
	# cd /tmp/kvsns_build/kvsns_shell
	# ./ns_init
4- test the namespace
	# use links from /tmp/kvsns_build/kvsns_shell to manipulate the namespace

IV. COMPILING GANESHA (sources under ~/nfs-ganesha
-----------------
1- Create build directory
	# mkdir /tmp/ganesha_build
2- run cmake and build ganesha
	# cd /tmp/ganesha_build
	# cmake ~/nfs-ganesha/src
	# make all rpms

V. CONFIGURING GANESHA (NO RPM)
-------------------
1- edit /etc/nfs-ganesha/kvsfs.ganesha.nfsd.conf, using this template
EXPORT
{
  # Export Id (mandatory)
  Export_Id = 77 ;
  Path = "/";
  FSAL {
    name = KVSFS;
    kvsns_config = /etc/kvsns.d/kvsns.ini;
  }
  Pseudo = /kvsns;
  Protocols=  NFSV3, 4, 9p;
  SecType = sys;
  MaxRead = 32768;
  MaxWrite = 32768;
  Filesystem_id = 192.168;
  Tag = temp;
  client {
        clients = *;
        Squash=no_root_squash;
        access_type=RW;
        protocols = 3, 4, 9p;
  }

}

FSAL
{
  KVSFS
   {
      FSAL_Shared_Library = /tmp/ganesha_build/FSAL/FSAL_KVSFS/libfsalkvsfs.so.4.2.0 ;
   }

}

FileSystem
{

  Link_support = TRUE;     # hardlink support
  Symlink_support = TRUE;  # symlinks support
  CanSetTime = TRUE;       # Is it possible to change file times
}

NFS_Core_Param
{
        Nb_Worker = 1 ;

        # NFS Port to be used
        # Default value is 2049
        NFS_Port = 2049 ;

        # Mount protocol RPC Program Number
        # Default value is 100005
        #MNT_Program = 100005 ;
        Protocols = 3, 4, 9p;

        Manage_Gids_Expiration = 3600;

        Plugins_Dir = /tmp/ganesha_build/FSAL/FSAL_KVSFS ;
}

NFSv4
{
  # Domain Name
  DomainName = localdomain ;

  # Lease_Lifetime = 10 ;
  Graceless = YES;
}

2- Run nfs-ganesha from the sources
	# cd /tmp/ganesha_build/MainNFSD
	# ./ganesha.nfsd -L /dev/tty -F -f ~/kvsfs.ganesha.nfsd.conf 

VI. CONFIGURING GANESHA (WITH RPM)
------------------------------
1- Install the nfs-ganesha and nfs-ganesha-kvsfs rpms
2- edit /etc/ganesha/ganesha.conf
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

3- start ganesha using systemctl
	# systemctl start nfs-gnesha



VII. MOUNT GANESHA ON CLIENT (not the same node as the one running ganesha)
-----------------------
1- Using NFSv4
	# mount -overs=4 client-21:/kvsns /mnt
2- Using NFSv4.1
	# mount -overs=4.1  client-21:/kvsns /mnt
3- Using NFSv3
	# mount -overs=3 client-21:/ /mnt

VIII. Enabling pNFS (server side)
---------------------------------
In this section, we will be using client-21 and client-22 as servers.
Node client-21 is the MDS (Metadata Server) and client-22 is a DS (Data Server).

1- Edit Ganesha config file on MDS to enable pNFS. 
The "Export" stanza is the only one to be touched.

EXPORT
{
  # Export Id (mandatory)
  Export_Id = 77 ;
  Path = "/";
  FSAL {
    name = KVSFS;
    kvsns_config = /etc/kvsns.d/kvsns.ini;
    PNFS {
       Stripe_Unit = 8192;
       pnfs_enabled = true;
       pnfs_enabled = false;
       Nb_Dataserver = 1;
        DS1 {
                DS_Addr = 172.18.1.22; # client-22
                DS_Port = 2049;
        }
    }
  }
  Pseudo = /pseudo;
  Protocols=  NFSV3, 4, 9p;
  SecType = sys;
  MaxRead = 32768;
  MaxWrite = 32768;
  Filesystem_id = 192.168;
  Tag = temp;
  client {
        clients = *;
        Squash=no_root_squash;
        access_type=RW;
        protocols = 3, 4, 9p;
  }

}

The DS will be using the same configuration file as the one shown in sections V and VI.

2- Start the DS and the MDS (see section V and VI).
3- Use modprobe to enable pNFS on the client side 
	# modprobe nfs_layout_nfsv41_files
4- Check that modules was correctely loaded
	# lsmod | grep nfs_layout
	nfs_layout_nfsv41_files    24018  0
	nfsv4                 546174  1 nfs_layout_nfsv41_files
	nfs                   256665  6 nfsv3,nfsv4,nfs_layout_nfsv41_files
	sunrpc                334343  26 nfs,nfsd,auth_rpcgss,lockd,nfsv3,nfsv4,nfs_acl,nfs_layout_nfsv41_files
5- Mount the MDS, using NFSv4.1 (pNFS is a NFSv4.1 feature, it does not exist
over NFSv4.0 or NFSv3)
	# mount -overs=4.1 client-21:/kvsns /mnt
