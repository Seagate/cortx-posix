# Troubleshooting Guide

## Motr

  * Run the below mentioned command to stop the motr single node service.
  
    * `sudo m0singlenode stop`
    
  * Run the below mentioned to perform the clean up action.
  
    * `m0setup -cv`
    
  * Run the below mentioned command to stop the lnet service.
  
    * `service lnet stop`
    
  * Run the below mentioned command to remove the lnet module.
  
    * `modprobe -r lnet`
		If there is a failure, you must reboot the machine.
		
  * Run the below mentioned command to load the module.
  
    * `modprobe lnet`
    
  * Start lnet service 
    * `service lnet start`
		Check if lnet is up and running using lctl command:
			* `lctl list_nids`
			output will be <ip-of-your-eth0>@tcp
  * `m0setup -cv`
  * `rm -fR /var/motr/*`
  * `sudo m0setup -A` 
  * `sudo m0setup -Mv -s128`
  * `sudo m0singlenode start`
