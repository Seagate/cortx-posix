
/* config.h file expanded by Cmake for build */

#ifndef CONFIG_H
#define CONFIG_H

#define GANESHA_VERSION_MAJOR 2
#define GANESHA_VERSION_MINOR 4
#define GANESHA_PATCH_LEVEL
#define GANESHA_EXTRA_VERSION -dev-9
#define GANESHA_VERSION "2.4-dev-9"

#define VERSION ".-dev-9"
#define VERSION_COMMENT "GANESHA file server is 64 bits compliant and supports NFS v3,4.0,4.1 (pNFS) and 9P"
#define _GIT_HEAD_COMMIT "99ac6d36ec44e979080aad0a774cc41dde7929d4"
#define _GIT_DESCRIBE "pre-2.0-dev_42-2914-g99ac6d3"
#define BUILD_HOST "localhost.localdomain"
#define FSAL_MODULE_LOC "/usr/lib64/ganesha"
/* Build controls */

/* #undef _MSPAC_SUPPORT */
/* #undef USE_NFSIDMAP */
/* #undef USE_DBUS */
/* #undef _USE_CB_SIMULATOR */
/* #undef USE_CAPS */
/* #undef USE_BLKID */
/* #undef PROXY_HANDLE_MAPPING */
#define _USE_9P 1
/* #undef _USE_9P_RDMA */
/* #undef _USE_NFS_RDMA */
/* #undef USE_FSAL_SHOOK */
/* #undef USE_FSAL_LUSTRE_UP */
/* #undef DEBUG_SAL */
/* #undef _VALGRIND_MEMCHECK */
/* #undef _NO_MOUNT_LIST */
#define HAVE_STDBOOL_H 1
#define HAVE_KRB5 1
#define KRB5_VERSION 194
/* #undef HAVE_HEIMDAL */
/* #undef USE_GSS_KRB5_CCACHE_NAME */
#define LINUX 1
/* #undef FREEBSD */
/* #undef APPLE */
#define _HAVE_GSSAPI 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRNLEN 1
#define LITTLEEND 1
/* #undef BIGEND */
/* #undef HAVE_XATTR_H */
/* #undef HAVE_INCLUDE_LUSTREAPI_H */
/* #undef HAVE_INCLUDE_LIBLUSTREAPI_H */
/* #undef _LUSTRE_API_HEADER */
#define HAVE_DAEMON 1
/* #undef USE_LTTNG */
/* #undef ENABLE_VFS_DEBUG_ACL */
/* #undef ENABLE_RFC_ACL */
/* #undef USE_GLUSTER_SYMLINK_MOUNT */

#define NFS_GANESHA 1

#define GANESHA_CONFIG_PATH "/etc/ganesha/ganesha.conf"
#define GANESHA_PIDFILE_PATH "/var/run/ganesha.pid"
#define NFS_V4_RECOV_ROOT "/var/lib/nfs/ganesha"
/**
 * @brief Default value for krb5_param.ccache_dir
 */
#define DEFAULT_NFS_CCACHE_DIR "/var/run/ganesha"

#endif /* CONFIG_H */
