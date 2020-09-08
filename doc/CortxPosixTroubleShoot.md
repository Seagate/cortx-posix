# Cortx-Posix TroubleShoot guide

## Motr troubleshoot

  * Stop motr singlenode service
    * `sudo m0singlenode stop`
  * Perform cleanup 
    * `m0setup -cv`
  * Stop lnet service
    * `service lnet stop`
  * Remove lnet module
    * `modprobe -r lnet`
		If it has failed then you need to reboot your machine
  * Load module
    * `modprobe lnet`
  * Start lnet service 
    * `service lnet start`
		Check if lnet is up and running using lctl command:
			* `lctl list_nids`
			output will be <ip-of-your-eth0>@tcp
  * `m0setup -cv`
  * `rm -fR /var/mero/*`
  * `sudo m0setup -A` 
  * `sudo m0setup -Mv -s128`
  * `sudo m0singlenode start`
