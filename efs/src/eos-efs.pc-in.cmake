prefix=/usr
exec_prefix=/usr/bin
libdir=/usr/lib64
includedir=/usr/include

Name: @PROJECT_NAME@
Description: EOS file system
Requires:
Version: @EOS_EFS_BASE_VERSION@
Cflags: -I/usr/include
Libs: -L/usr/lib64 -leos-efs

