#!/usr/bin/python3

#
# Filename:         commands.py
# Description:      Command descriptor and parser utilities for efs-cli.
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

import argparse

class Command(object):
	"""
	Base class for all commands supported by EFS CLI
	"""

	def __init__(self, args):
		self._args = args

	def action(self):
		return self._args.action

	def args(self):
		return self._args

class FsCommand(Command):
	"""

	Contains functionality to handle support bundle.
	"""

	def __init__(self, args):
		super().__init__(args)

	def name(self): 
		return "fs"

	@staticmethod
	def add_args(parser):
		sbparser = parser.add_parser("fs", help='create, list or delete FS.')
		sbparser.add_argument('action', help='action', choices=['create', 'list', 'delete'])
		sbparser.add_argument('args', nargs='*', default=[], help='fs command options')
		sbparser.set_defaults(command=FsCommand)

class CommandFactory(object):
	"""
	Factory for representing and creating command objects
	using a generic skeleton.
	"""

	commands = {FsCommand}

	def get_command(argv):
		"""
		Parse the command line as per the syntax and return command.
		"""

		parser = argparse.ArgumentParser(description='EFS CLI command')
		subparsers = parser.add_subparsers()

		for command in CommandFactory.commands:
			command.add_args(subparsers)

		try:
			args = parser.parse_args(argv)
			command = args.command(args)
		except Exception as e:
			raise Exception('invalid command. %s' %e)

		return command
