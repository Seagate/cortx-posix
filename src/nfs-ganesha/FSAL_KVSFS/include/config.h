/* config.h file expanded by Cmake for build */
/* TODO:
 *	 This file was taken from a local build dir.
 *	 We need to add a script which will copy this file
 *	 automaticaly from a build dir specified during the
 *	 configuration step.
 */
#ifndef CONFIG_H
#define CONFIG_H

#define GANESHA_VERSION_MAJOR 2
#define GANESHA_VERSION_MINOR 7
#define GANESHA_PATCH_LEVEL .6
#define GANESHA_EXTRA_VERSION 
#define GANESHA_VERSION "2.7.6"
#define GANESHA_BUILD_RELEASE 0

#define VERSION GANESHA_VERSION
#define VERSION_COMMENT "GANESHA file server is 64 bits compliant and supports NFS v3,4.0,4.1 (pNFS) and 9P"
#define _GIT_HEAD_COMMIT "affd5942cd68b83386c575929608ae0d0c476b62"
#define _GIT_DESCRIBE "V2.7.6-0-gaffd594"
#define BUILD_HOST "qb21n1-m05-vm3"
#define FSAL_MODULE_LOC "/usr/lib64/ganesha"
/* Build controls */

/* #undef _MSPAC_SUPPORT */
#define USE_NFSIDMAP 1
#define USE_DBUS 1
/* #undef _USE_CB_SIMULATOR */
#define USE_CAPS 1
#define USE_BLKID 1
/* #undef PROXY_HANDLE_MAPPING */
#define _USE_9P 1
/* #undef _USE_9P_RDMA */
/* #undef _USE_NFS_RDMA */
#define _USE_NFS3 1
#define _USE_NLM 1
/* #undef DEBUG_SAL */
/* #undef _VALGRIND_MEMCHECK */
/* #undef _NO_TCP_REGISTER */
#define RPCBIND 1
#define HAVE_KRB5 1
/* #undef HAVE_HEIMDAL */
/* #undef USE_GSS_KRB5_CCACHE_NAME */
#define LINUX 1
/* #undef FREEBSD */
#define _HAVE_GSSAPI 1
#define HAVE_STRING_H 1
#define HAVE_STRNLEN 1
#define LITTLEEND 1
#define HAVE_DAEMON 1
/* #undef USE_LTTNG */
/* #undef ENABLE_VFS_DEBUG_ACL */
/* #undef ENABLE_RFC_ACL */
/* #undef USE_GLUSTER_XREADDIRPLUS */
/* #undef USE_GLUSTER_UPCALL_REGISTER */
/* #undef USE_FSAL_CEPH_MKNOD */
/* #undef USE_FSAL_CEPH_SETLK */
/* #undef USE_FSAL_CEPH_LL_LOOKUP_ROOT */
/* #undef USE_FSAL_CEPH_STATX */
/* #undef USE_FSAL_CEPH_LL_DELEGATION */
/* #undef USE_FSAL_CEPH_LL_SYNC_INODE */
/* #undef USE_CEPH_LL_FALLOCATE */
/* #undef USE_FSAL_CEPH_ABORT_CONN */
/* #undef USE_FSAL_CEPH_RECLAIM_RESET */
/* #undef USE_FSAL_CEPH_GET_FS_CID */
/* #undef USE_FSAL_RGW_MOUNT2 */
/* #undef ENABLE_LOCKTRACE */
/* #undef SANITIZE_ADDRESS */
/* #undef DEBUG_MDCACHE */
/* #undef USE_RADOS_RECOV */
/* #undef RADOS_URLS */
#define USE_LLAPI 1
/* #undef USE_GLUSTER_STAT_FETCH_API */
#define NFS_GANESHA 1

#define GANESHA_CONFIG_PATH "/etc/ganesha/ganesha.conf"
#define GANESHA_PIDFILE_PATH "/var/run/ganesha.pid"
#define NFS_V4_RECOV_ROOT "/var/lib/nfs/ganesha"
/**
 * @brief Default value for krb5_param.ccache_dir
 */
#define DEFAULT_NFS_CCACHE_DIR "/var/run/ganesha"

/* We're LGPL'd */
#define _LGPL_SOURCE 1

#endif /* CONFIG_H */
