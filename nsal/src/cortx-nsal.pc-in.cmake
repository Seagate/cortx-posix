prefix=/usr
exec_prefix=/usr/bin
libdir=/usr/lib64
includedir=/usr/include

Name: @PROJECT_NAME@
Description: Namespace abstraction layer library
Requires:
Version: @CORTX_NSAL_BASE_VERSION@
Cflags: -I/usr/include
Libs: -L/usr/lib64 -l@PROJECT_NAME@

