#!/usr/bin/python3

#
# Filename:         efscli.py
# Description:      Command line interface to manage EFS.
#
# Do NOT modify or remove this copyright and confidentiality notice!
# Copyright (c) 2019, Seagate Technology, LLC.
# The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
# Portions are also trade secret. Any use, duplication, derivation,
# distribution or disclosure of this code, for any reason, not expressly
# authorized is prohibited. All other rights are expressly reserved by
# Seagate Technology, LLC.
#
# Author: Yogesh.lahane@seagate.com
#

import sys
import json
import argparse
import os
import logging
from commands import *
from client import *

def main(argv):
	"""
	Parse and execute command line to obtain command structure.
	Execute the CLI
	"""
	cli_path = os.path.realpath(argv[0])
	sys.path.append(os.path.join(os.path.dirname(cli_path), '..', '..'))

	try:
		command = CommandFactory.get_command(argv[1:])
		if command == None:
			return 1
		# Create HttpReques
		request = HttpRequest(command)

		# Create RestClient Instance.
		host = "127.0.0.1"
		port = "8081"
		client = RestClient(host, port)

		# Process the request
		resp = client.process(request)
		print(resp.status, resp.reason)
		if len(resp.body) != 0:
			print(resp.body)
		return 0
	except Exception as exception:
		sys.stderr.write('%s\n' %exception)
		return 1

if __name__ == '__main__':
	sys.exit(main(sys.argv)) 
