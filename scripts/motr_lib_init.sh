#!/bin/bash

# Index info
PROFILE='<0x7000000000000001:0>'
PROC_FID='<0x7200000000000000:0>'
LOC_EXPORT_ID='@tcp:12345:44:301'
HA_EXPORT_ID='@tcp:12345:45:1'
KVS_GLOBAL_FID='<0x780000000000000b:1>'
KVS_NS_META_FID='<0x780000000000000b:2>'
INDEX_DIR=/tmp
CORTXFS_CONF=/etc/cortx/cortxfs.conf
CORTXFS_CONF_BAK=${CORTXFS_CONF}.$$

die() {
	echo "Error: $*"
	exit 1
}

prepare_cortx_fs_conf () {
	# Backup cortxfs.conf file
	[ ! -e $CORTXFS_CONF_BAK ] && cp $CORTXFS_CONF $CORTXFS_CONF_BAK

	# Modify cortxfs.conf
	tmp_var=$(sed -n '/\[log\]/ {=;q}' $CORTXFS_CONF)
	[ $? -ne 0 ] && die "Failed to access cortxfs.conf file"

	sed -i "$tmp_var,\$d" $CORTXFS_CONF
	[ $? -ne 0 ] && die "Failed to edit cortxfs.conf file"

	cat >> $CORTXFS_CONF << EOM
[log]
path = /var/log/cortx/fs/cortxfs.log
level = LEVEL_INFO

[kvstore]
type = cortx
ns_meta_fid = $KVS_NS_META_FID

[dstore]
type = cortx

[motr]
local_addr = $ip_add$LOC_EXPORT_ID
ha_addr = $ip_add$HA_EXPORT_ID
profile = $PROFILE
proc_fid = $PROC_FID
index_dir = $INDEX_DIR
kvs_fid = $KVS_GLOBAL_FID
EOM
	[ $? -ne 0 ] && die "Failed to configure cortxfs.conf"

	echo "Configuration Completed"
}

prerequisites() {
	# Check motr-kernel
	systemctl is-active motr-kernel > /dev/null 2>&1
	[ $? -ne 0 ] && die "motr-kernel is not active"

	# Stop nfs-ganesha service if running
	systemctl stop nfs-ganesha
	[ $? -ne 0 ] && die "Falied to stop nfs-ganesha service"

	# Get ip address
	v1=$(lctl list_nids 2> /dev/null)
	ip_add=${v1::-4}
}

prepare_index() {
	# Use existing indexes
	[ -n "$USE_IDX" ] && return
 
	# Drop indexes
	m0kv -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE\
		-f $PROC_FID index drop "$KVS_GLOBAL_FID" > /dev/null 2>&1
	[ $? -ne 0 ] && die "Failed to drop Global index"

	m0kv -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE\
		-f $PROC_FID index drop "$KVS_NS_META_FID" > /dev/null 2>&1
	[ $? -ne 0 ] && die "Failed to drop NS_META index"

	# Create indexes
	m0kv -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE\
		-f $PROC_FID index create "$KVS_GLOBAL_FID" > /dev/null 2>&1
	[ $? -ne 0 ] && die "Failed to create Global index"

	m0kv -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE\
		-f $PROC_FID index create "$KVS_NS_META_FID" > /dev/null 2>&1
	[ $? -ne 0 ] && die "Failed to create NS_META index"

	echo "Clean indexes prepared"
}

usage() {
	cat <<EOM
usage: $0 {setup|conf|idx-gen} [-h] [-e]
Commands:
   -h         Help
   -e         Use existing indexes
   setup      Configure cortxfs.conf and prepare indexes
   conf       Configure cortxfs.conf
   idx-gen    Prepare clean indexes

Default values used for Index creation are-
   Profile:               $PROFILE
   Process FID:           $PROC_FID
   Local Export Suffix:   $LOC_EXPORT_ID
   HA Export Suffix:      $HA_EXPORT_ID
   KVS Global FID:        $KVS_GLOBAL_FID
   KVS NS Meta FID:       $KVS_NS_META_FID
EOM
	exit 1
}

#main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

cmd=$1; shift 1

getopt --options "he" --name motr_lib_init
[ $? -ne 0 ] && usage

while [ ! -z $1 ]; do
	case "$1" in
	-h ) usage;;
	-e ) USE_IDX=1;;
	*  ) usage;;
	esac
	shift 1
done

case $cmd in
	setup   ) prerequisites; prepare_cortx_fs_conf; prepare_index;;
	conf    ) prerequisites; prepare_cortx_fs_conf;;
	idx-gen	) prerequisites; prepare_index;;
	*       ) usage;;
esac
