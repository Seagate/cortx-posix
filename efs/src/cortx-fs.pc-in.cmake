prefix=/usr
exec_prefix=/usr/bin
libdir=/usr/lib64
includedir=/usr/include

Name: @PROJECT_NAME@
Description: CORTX file system
Requires:
Version: @CORTX_EFS_BASE_VERSION@
Cflags: -I/usr/include
Libs: -L/usr/lib64 -l@PROJECT_NAME@

