#!/bin/bash
# Variables
PROFILE='<0x7000000000000001:0>'
PROC_FID='<0x7200000000000000:0>'
INDEX_DIR=/tmp
# Global idx (meta index)
KVS_GLOBAL_FID='<0x780000000000000b:1>'
KVS_NS_META_FID='<0x780000000000000b:2>'
DEFAULT_FS=''
FS_PATH='nonexistent'
LOC_EXPORT_ID='@tcp:12345:44:301'
HA_EXPORT_ID='@tcp:12345:45:1'
CORTXFS_CONF=/etc/cortx/cortxfs.conf
CORTXFS_CONF_BAK=${CORTXFS_CONF}.$$
GANESHA_CONF=/etc/ganesha/ganesha.conf
GANESHA_CONF_BAK=${GANESHA_CONF}.$$
EFS_FS_CLI="/usr/bin/efscli"
NFS_INITIALIZED=/var/lib/nfs/nfs_initialized
NFS_SETUP_LOG=/var/log/nfs_setup.log
DEFAULT_EXPORT_OPTION="proto=nfs,secType=sys,Filesystem_id=192.1,client=1,"
DEFAULT_EXPORT_OPTION+="clients=*,Squash=no_root_squash,access_type=RW,protocols=4"

function die {
	log "error: $*"
	echo "error: $*"
	exit 1
}

function run {
	echo -ne "\n$ $*"
	[ ! -z "$prompt" ] && read a &&[ "$a" = "c" -o "$a" = "C" ] && exit 1
		$*
	return $?
}

function log {
	echo "$*" >> $NFS_SETUP_LOG
}

function create_fs {
	echo -e "\nCreating default file system $DEFAULT_FS ..."
	run $EFS_FS_CLI fs create $DEFAULT_FS
	[ $? -ne 0 ] && die "Failed to create $DEFAULT_FS"
	echo -e "\nExporting default file system $DEFAULT_FS $DEFAULT_EXPORT_OPTION ..."
	run $EFS_FS_CLI endpoint create $DEFAULT_FS $DEFAULT_EXPORT_OPTION
	[ $? -ne 0 ] && die "Failed to create export $DEFAULT_FS "
}

function get_ip {
	# Get ip address
	v1=$(lctl list_nids 2> /dev/null)
	v2=${v1::-4}
        echo "$v2"
}

function clovis_init {
	log "Initializing Clovis..."

	# Create Clovis global idx
	run m0clovis -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE -f $PROC_FID index create "$KVS_GLOBAL_FID"
	# Create Clovis fs_meta idx
	run m0clovis -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE -f $PROC_FID index create "$KVS_NS_META_FID"

	[ $? -ne 0 ] && die "Failed to Initialise Clovis Global index"
}

function efs_init {
	log "Initializing EFS..."

	# Backup cortxfs.conf file
	[ ! -e $CORTXFS_CONF_BAK ] && run cp $CORTXFS_CONF $CORTXFS_CONF_BAK

	# Modify cortxfs.conf
	tmp_var=$(sed -n '/\[log\]/ {=;q}' $CORTXFS_CONF)
	[ $? -ne 0 ] && die "Failed to access cortxfs.conf file"

	run sed -i "$tmp_var,\$d" $CORTXFS_CONF
	[ $? -ne 0 ] && die "Failed to edit cortxfs.conf file"

	cat >> $CORTXFS_CONF << EOM
[log]
path = /var/log/cortx/fs/cortxfs.log
level = LEVEL_INFO

[kvstore]
type = eos
ns_meta_fid = $KVS_NS_META_FID

[dstore]
type = eos

[mero]
local_addr = $ip_add$LOC_EXPORT_ID
ha_addr = $ip_add$HA_EXPORT_ID
profile = $PROFILE
proc_fid = $PROC_FID
index_dir = $INDEX_DIR
kvs_fid = $KVS_GLOBAL_FID
EOM
	[ $? -ne 0 ] && die "Failed to configure cortxfs.conf"

	touch $NFS_INITIALIZED


}

function prepare_ganesha_conf {
	log "Preparing NFS Ganesha configuration..."

	# Backup ganesha.conf file
	[ ! -e  $GANESHA_CONF_BAK ] && run cp $GANESHA_CONF $GANESHA_CONF_BAK

	# Configure NFS-Ganesha
	cat > $GANESHA_CONF << EOM
# An example of KVSFS NFS Export
EXPORT {

	# Export Id (mandatory, each EXPORT must have a unique Export_Id)
	Export_Id = 12345;

	# Exported path (mandatory)
	Path = $FS_PATH;

	# Pseudo Path (required for NFSv4 or if mount_path_pseudo = true)
	Pseudo = /$FS_PATH;

	# Exporting FSAL
	FSAL {
		Name  = CORTX-FS;
		cortxfs_config = $CORTXFS_CONF;
	}

	# Allowed security types for this export
	SecType = sys;

	Filesystem_id = 192.168;

	client {
		clients = *;

		# Whether to squash various users.
		Squash=no_root_squash;

		# Access type for clients.  Default is None, so some access must be
		# given. It can be here, in the EXPORT_DEFAULTS, or in a CLIENT block
		access_type=RW;

		# Restrict the protocols that may use this export.  This cannot allow
		# access that is denied in NFS_CORE_PARAM.
		protocols = 4;
	}
}

# KVSFS Plugin path
FSAL {
	CORTX-FS {
		FSAL_Shared_Library = /usr/lib64/ganesha/libfsalcortx-fs.so.4.2.0 ;
	}
}

NFS_Core_Param {
	Nb_Worker = 1 ;
	Manage_Gids_Expiration = 3600;
	Plugins_Dir = /usr/lib64/ganesha/ ;
}

NFSv4 {
	# Domain Name
	DomainName = localdomain ;

	# Quick restart
	Graceless = YES;
}
EOM
	[ $? -ne 0 ] && die "Failed to Configure NFS Ganesha"
}

function check_prerequisites {
	lctl list_nids > /dev/null 2>&1 || die "Mero not active"
	# Check mero
	tmp_var=$(rpm --version mero)
	[ -z "$tmp_var" ] && die "Mero RPMs not installed"

	# Check mero status
	tmp_var=$(systemctl is-active mero-kernel)
	[[ "$tmp_var" -ne "active" ]] && die "Mero-kernel is inactive"

	# Check mero services
	tmp_var=$(pgrep m0)
	[ -z "$tmp_var" ] && die "Mero services not activate"

	#check SElinux status
	tmp_var="$(getenforce)"
	if [ "$tmp_var" == "Enforcing" ]; then
		die "EOS NFS cannot work with SELinux enabled. Please disable it using \"setenforce Permissive\""
	fi

	# Check nfs-ganesha
	tmp_var=$(rpm --version nfs-ganesha)
	[ -z "$tmp_var" ] && die "NFS RPMs not installed "
}

function eos_nfs_init {
	# Check if NFS is already initialized
	[[ -e $NFS_INITIALIZED && -z "$force" ]] &&
		[ "$(cat $NFS_INITIALIZED)" = "success" ] && die "NFS already initialzed"

	# Cleanup before initialization
	eos_nfs_cleanup

	# Initialize clovis
	clovis_init

	efs_init

	# Prepare ganesha.conf
	prepare_ganesha_conf

	# Start NFS Ganesha Server
	systemctl restart nfs-ganesha || die "Failed to start NFS-Ganesha"
	#[ $? -ne 0 ] && die "Failed to start NFS-Ganesha"

	# Create default FS
	if [ -n "$DEFAULT_FS" ]; then
	create_fs
	systemctl restart nfs-ganesha || die "Failed to start NFS-Ganesha"
	fi

	echo success > cat $NFS_INITIALIZED
	echo -e "\nNFS setup is complete"
}

function eos_nfs_cleanup {
	# Stop nfs-ganesha service if running
	systemctl status nfs-ganesha > /dev/null && systemctl stop nfs-ganesha

	# Drop index if previosly created
	run m0clovis -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE -f $PROC_FID index drop "$KVS_GLOBAL_FID"
	run m0clovis -l $ip_add$LOC_EXPORT_ID -h $ip_add$HA_EXPORT_ID -p $PROFILE -f $PROC_FID index drop "$KVS_NS_META_FID"

	rm -f $NFS_INITIALIZED
	echo "NFS cleanup is complete"
}

function usage {
	cat <<EOF
usage: $0 {init|cleanup} [-h] [-f] [-p] [-P <Profile>] [-F <Process FID>] [-k <KVS FID>] [-e <Local export>] [-E <HA export>]
options:
  -h help
  -f force initialization
  -p prompt
  -P Profile. Default is <0x7000000000000001:0>
  -F Process FID. Default is <0x7200000000000000:0>
  -k KVS FID. Default is <0x780000000000000b:1>
  -e Local Export Suffix. Default is @tcp:12345:44:301
  -E HA Export Suffix. Default is @tcp:12345:45:1
EOF
	exit 1
}

# Main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

cmd=$1; shift 1

getopt --options "hfpP:F:k:e:E:d:" --name nfs_setup
[[ $? -ne 0 ]] && usage

while [ ! -z $1 ]; do
	case "$1" in
		-h ) usage;;
		-f ) force=1;;
		-p ) prompt=1;;
		-P ) PROFILE=$2; shift 1;;
		-F ) PROC_FID=$2; shift 1;;
		-k ) KVS_GLOBAL_FID=$2; shift 1;;
		-e ) LOC_EXPORT_ID=$2; shift 1;;
		-E ) HA_EXPORT_ID=$2; shift 1;;
		-d ) DEFAULT_FS=$2; shift 1;;
		 * ) usage ;;
	esac
	shift 1
done

if [ ! -e $EFS_FS_CLI ]; then
  die "efscli is not installed in the location $EFS_FS_CLI. Please install the corresponding RPM."
fi

# Get path and Pseudo path
[ -n "$DEFAULT_FS" ] && FS_PATH="$DEFAULT_FS"

# Getip address
ip_add=$(get_ip)
#check for lent
[ -z $ip_add ] && die "Could not determine IP address. Please ensure lnet is configured !!!"

case $cmd in
	init    ) check_prerequisites; eos_nfs_init;;
	cleanup ) check_prerequisites; eos_nfs_cleanup;;
	*       ) usage;;
esac
