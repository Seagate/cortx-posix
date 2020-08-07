#!/bin/bash

# Variables
PROFILE="0x7000000000000001:0"
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
	ip_add=${v1::-4}

	[ -n "$PROVI_SETUP" ] && return

	# Set LOC_EP and HA_EP for Dev env
	LOC_EP="$ip_add$LOC_EXPORT_ID"
	HA_EP="$ip_add$HA_EXPORT_ID"
}

function get_ep {
	[ -n "$EP" ] && return || EP=1

	host_name=$(hostname -f )
	hctl_status=$(hctl status --json | jq '.')

	cluster_info=$(echo $hctl_status | jq '.nodes[]')
	node_info=$(echo $cluster_info | jq --arg h_name $host_name '. | select(.name == $h_name)')

	PROFILE=$(echo $hctl_status | jq '.profile' | sed 's/[,"]//g')
	HA_EP=$(echo $node_info | jq '.svcs[] | select(.name == "hax") | .ep' | sed 's/[,"]//g')

	m0client_info=$(echo $node_info | jq '[.svcs[] | select(.name == "m0_client")][0]')
	LOC_EP=$(echo $m0client_info | jq '.ep' | sed 's/[,"]//g')
	PROC_FID=$(echo $m0client_info | jq '.fid' | sed 's/[,"]//g')
}

function clovis_init {
	log "Initializing Clovis..."

	# Create Clovis global idx
	run m0clovis -l $LOC_EP -h $HA_EP -p $PROFILE -f $PROC_FID index create\
		"$KVS_GLOBAL_FID"
	# Create Clovis fs_meta idx
	run m0clovis -l $LOC_EP -h $HA_EP -p $PROFILE -f $PROC_FID index create\
		"$KVS_NS_META_FID"

	[ $? -ne 0 ] && die "Failed to Initialise Clovis Global index"
}

function prepare_fs_conf {
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
local_addr = $LOC_EP
ha_addr = $HA_EP
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
	# Check prerequisites only once
	[ -n "$PRE_REQ" ] && return || PRE_REQ=1

	# Prerequisites specifc to Dev environment
	if [ -z "$PROVI_SETUP" ]; then
		# Check lnet status
		lctl list_nids > /dev/null 2>&1 || die "lnet not active"

		# Check cortx-motr rpms
		rpm -q cortx-motr > /dev/null 2>&1
		[[ $? -ne 0 ]] && die "cortx-motr RPMs not installed"

		# Check cortx-motr kernel status
		tmp_var=$(systemctl is-active motr-kernel)
		[[ "$tmp_var" != "active" ]] && die "cortx-motr kernel is inactive"

		# Check cortx-motr services
		tmp_var=$(pgrep m0)
		[ -z "$tmp_var" ] && die "cortx-motr services not activate"
	fi	

	# Check SElinux status
	tmp_var="$(getenforce)"
	if [ "$tmp_var" == "Enforcing" ]; then
		die "EOS NFS cannot work with SELinux enabled. Please disable \
			it using \"setenforce Permissive\""
	fi

	# Check nfs-ganesha
	tmp_var=$(rpm --version nfs-ganesha)
	[ -z "$tmp_var" ] && die "NFS RPMs not installed "
}

function cortx_nfs_init {
	[ -n "$PROVI_SETUP" ] && get_ep

	# Cleanup before initialization
	cortx_nfs_cleanup

	# Initialize clovis
	clovis_init

	echo -e "\nNFS Initialization is complete"
}

function cortx_nfs_config {
	[ -n "$PROVI_SETUP" ] && get_ep

	# Check if NFS is already initialized
	[[ -e $NFS_INITIALIZED && -z "$force" ]] &&
		[ "$(cat $NFS_INITIALIZED)" = "success" ] && die "NFS already initialzed"

	# Check prerequisites
	check_prerequisites

	# Prepare cortxfs.conf
	prepare_fs_conf

	# Prepare ganesha.conf
	prepare_ganesha_conf

	# Start NFS Ganesha Server
	systemctl restart nfs-ganesha || die "Failed to start NFS-Ganesha"

	# Create default FS
	if [ -n "$DEFAULT_FS" ]; then
		create_fs
		systemctl restart nfs-ganesha || die "Failed to start NFS-Ganesha"
	fi

	echo success > cat $NFS_INITIALIZED
	echo -e "\nNFS setup is complete"
}

function cortx_nfs_cleanup {
	[ -n "$PROVI_SETUP" ] && get_ep

	# Check prerequisites
	check_prerequisites

	# Stop nfs-ganesha service if running
	systemctl status nfs-ganesha > /dev/null && systemctl stop nfs-ganesha

	# Drop index if previosly created
	run m0clovis -l $LOC_EP -h $HA_EP -p $PROFILE -f $PROC_FID index drop\
		"$KVS_GLOBAL_FID"
	run m0clovis -l $LOC_EP -h $HA_EP -p $PROFILE -f $PROC_FID index drop\
		"$KVS_NS_META_FID"

	rm -f $NFS_INITIALIZED
	echo "NFS cleanup is complete"
}

function usage {
	cat <<EOF
usage: $0 {init|config|setup|cleanup} [-h] [-f] [-p] [-q] [-d <FS name>]
Command:
  init      Create clovis indexes
  config    Prepare conf files and start NFS-Ganesha
  setup     Complete setup: init and config command
  cleanup   Stop NFS-Ganesha and drop indexes
Options:
  -h Help
  -f Force initialization
  -p Prompt
  -q To perform Setup on Provisioner VM
  -d To create FS

Default values used for Index creation on Dev env are-
   Profile:               <0x7000000000000001:0>
   Process FID:           <0x7200000000000000:0>
   Local Export Suffix:   @tcp:12345:44:301
   HA Export Suffix:      @tcp:12345:45:1
   KVS Golbal FID:        <0x780000000000000b:1>
   KVS NS Meta FID:       <0x780000000000000b:2>
EOF
	exit 1
}

# Main

[ $(id -u) -ne 0 ] && die "Run this script with root privileges"

cmd=$1; shift 1

getopt --options "hfpqd:" --name nfs_setup
[[ $? -ne 0 ]] && usage

while [ ! -z $1 ]; do
	case "$1" in
		-h ) usage;;
		-f ) force=1;;
		-p ) prompt=1;;
		-q ) PROVI_SETUP=1;;
		-d ) DEFAULT_FS=$2; shift 1;;
		 * ) usage ;;
	esac
	shift 1
done

if [ ! -e $EFS_FS_CLI ]; then
	die "efscli is not installed in the location $EFS_FS_CLI. Please install\
		the corresponding RPM."
fi

# Get path and Pseudo path
[ -n "$DEFAULT_FS" ] && FS_PATH="$DEFAULT_FS"

# Getip address
get_ip
#check for lent
[ -z $ip_add ] && die "Could not determine IP address. Please ensure lnet is\
	configured !!!"

case $cmd in
	init    ) cortx_nfs_init;;
	config	) cortx_nfs_config;;
	setup	) cortx_nfs_init; cortx_nfs_config;;
	cleanup ) cortx_nfs_cleanup;;
	*       ) usage;;
esac
