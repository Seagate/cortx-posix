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
import http
from http import HTTPStatus
import http.client
import json
import textwrap
import sys, traceback
import re

'''
efscli tool validation rules:
efscli tool includes a valiation rules system to process the passed parameters.
The endpoint operations use this for existing passed parameters.
Any new operation and it's args must be updated and validated in this rule from
now on.

The validation definition is embedded in this source file, also can be placed
in an external JSON file and passed by user.
However the external JSON file must not be exposed to customers and should be
disabled in production builds.
The external JSON config can also be used to perform negative tests for the REST
server.
Note: For the time being it is only being used by for endpoint commands.
Priority:
	1. If local dev version exists, i.e. efscli_validation_rule_dev
	2. If user passed an existing file
	3. Embedded
The dev version of the rules stored in efscli_validation_rule_dev is usually in
the same dir as the efscli.conf in source tree. Run efscli from the script's
source dir to use local dev rule config automatically.
Validation rules are based on nfs-ganesha-eos/src/config_samples/config.txt
and the rules will need update in future.
'''

efscli_validation_rule_dev='./efscli_validation_rules.json'
# Don't allow any special chars for FS names, except '-', '_'
fs_name_regex="[^A-Za-z0-9/]"
fs_name_max_len=255
efscli_default_validation_rules = """{
	"nfs" : {
			"proto" : {"str": "nfs"},
			"status" : {"set" : "enabled,disabled"},
			"secType" : {"set" : "none,sys"},
			"Filesystem_id" : {"regex" : "[^0-9+.0-9+$]", "limit" : "100"},
			"client" : {"max_count" : "10"},
			"clients" : {"regex" : "[^A-Za-z0-9.*/]", "limit" : "100"},
			"Squash" : {"set" : "no_root_squash,root_squash"},
			"access_type" : {"set" : "RW,R,W,None"},
			"protocols" : {"set" : "4,4.1"}
	},

	"smb" : {
			"proto" : {"str" : "smb"},
			"status" : {"set" : "unsupported"}
	}
}"""

def throw_exception_with_msg(err_msg):
	raise Exception(format(err_msg))

def regex_pattern_check(regex_pattern, len_limit, arg):
	if (type(arg) != str) or (type(regex_pattern) != str) or\
		(type(len_limit) != int):
			throw_exception_with_msg("Incorrect type passed" + type(arg),\
			+ type(regex_pattern) + type(len_limit))
	elif (len(arg) > len_limit):
			return False
	return not bool (re.compile(regex_pattern).search(arg))

def read_conf_file(conf_file):
		with open(conf_file, 'r') as conf_file_handle:
			json_data = json.loads(conf_file_handle.read())
		return json_data

def validate_key_val(inp_key, inp_val, conf_vals):
	for conf_key, conf_val in conf_vals.items():
		if conf_key == 'str':
			if inp_val != conf_val:
				throw_exception_with_msg("Invalid:" + inp_key + "=" + inp_val\
				+ ", allowed only: " + conf_val)
		elif conf_key == 'max_count':
			if int(inp_val) > int(conf_vals['max_count']):
				throw_exception_with_msg("Invalid:" + inp_key + "=" + inp_val\
				+ ", value must be less than or equals to: " +\
				conf_vals['max_count'])
		elif conf_key == 'set':
			conf_set_vals = conf_val.split(',')
			if inp_val not in conf_set_vals:
				throw_exception_with_msg("Invalid:" + inp_key + "=" + inp_val\
				+ ", value must be only from:" + str(conf_set_vals))
		elif conf_key == 'regex':
				if regex_pattern_check(conf_vals['regex'],\
				int(conf_vals['limit']), inp_val) != True:
					throw_exception_with_msg("Invalid:" + inp_key + "=" + \
					inp_val + "must use regex:" +  conf_vals)

def validate_inp_config_params(conf_data, inp_args):
	inp_params = {}
	option_list = inp_args.split(',')

	for option_token in option_list:
		option = option_token.split('=')
		inp_params[option[0]] = option[1]

	conf_params = conf_data[inp_params['proto']]
	for inp_key, inp_val in inp_params.items():
		conf_vals = conf_params[inp_key]
		validate_key_val(inp_key, inp_val, conf_vals)

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

	def validate_args_payload(self, args):
		return

class FsCommand(Command):
	"""
	Contains functionality to handle FS commands.
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

	def validate_args_payload(self, args):
		if args.action.lower() == 'list' and len(args.args) != 0:
			throw_exception_with_msg("Too many args for " + \
			args.action.lower())

		if args.action.lower() == 'create' or args.action.lower() == 'delete':
			if len(args.args) != 1:
				throw_exception_with_msg("Too many or no args for " + \
				args.action.lower())
			# arg[0] is FS name, check it
			fs = args.args[0]
			if regex_pattern_check(fs_name_regex, fs_name_max_len, fs) != True:
				throw_exception_with_msg("Invalid FS param: " + fs +\
				", allowed regex:" + fs_name_regex + ", allowed max len:"\
				+ str(fs_name_max_len))

class EndpointCommand(Command):
	"""
	Contains functionality to handle EXPORT commands.
	"""

	def __init__(self, args):
		super().__init__(args)

	def name(self):
		return "endpoint"

	@staticmethod
	def add_args(parser):
		sbparser = parser.add_parser("endpoint", help='create, delete and update Endpoint.')
		sbparser.add_argument('-cv', '--config_validation',
			help='Optional JSON config file for validation rules.',\
			default=efscli_validation_rule_dev)
		sbparser.add_argument('action', help='action', choices=['create', 'delete', 'update'])
		sbparser.add_argument('args', nargs='*', default=[],\
			help='Endpoint command options.')
		sbparser.set_defaults(command=EndpointCommand)

	def validate_args_payload(self, args):
		'''
		Note:
		Production build should not have any option to pass custom
		validation rules to this tool. Disable them!
		Must only use the embedded rules.
		Priority:
			1. If local dev version exists, i.e. efscli_validation_rule_dev 
			2. If user passed an existing file
			3. Embedded
		''' 
		if os.path.isfile(efscli_validation_rule_dev) == True:
			print ("Using dev validation rules")
			efscli_config_rules = read_conf_file(efscli_validation_rule_dev)
		elif os.path.isfile(args.config_validation) == True:
			print ("Using external validation rules")
			efscli_config_rules = read_conf_file(args.config_validation)
		else:
			print ("Using embedded validation rules")
			efscli_config_rules = json.loads(efscli_default_validation_rules)

		if args.action.lower() == 'delete':
			if len(args.args) != 1:
				throw_exception_with_msg("Too many or no args for " + \
				args.action.lower())
			if regex_pattern_check(fs_name_regex, fs_name_max_len, \
				args.args[0]) != True:
				throw_exception_with_msg("Invalid FS param: " + args.args[0] +\
				", allowed regex:" + fs_name_regex + ", allowed max len:"\
				+ str(fs_name_max_len))

		if args.action.lower() == 'create':
			if len(args.args) != 2:
				throw_exception_with_msg("Too many or no args for " + \
					args.action.lower())
			# arg[0] is FS name, check it
			fs = args.args[0]
			if regex_pattern_check(fs_name_regex, fs_name_max_len, fs) != True:
				throw_exception_with_msg("Invalid FS param: " + fs +\
				", allowed regex:" + fs_name_regex + ", allowed max len:"\
				+ str(fs_name_max_len))

			validate_inp_config_params(efscli_config_rules, args.args[1])

class CommandFactory(object):
	"""
	Factory for representing and creating command objects
	using a generic skeleton.
	"""

	commands = {FsCommand, EndpointCommand}

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
			command.validate_args_payload(args)
		except Exception as e:
			traceback.print_exc(file=sys.stderr)
			raise Exception('Command Validation failed. %s' %e)

		return command

class Request:
	"""
	Represents a request which is processed by a client.
	"""

	def __init__(self, command):
		self._command = command

	def command(self):
		return self._command.name()

	def action(self):
		return self._command.action()

	def args(self):
		params = self._command.args()
		return params.args;

class Response:
	"""
	Represents a response for the Request.
	"""
	codes = {
		HTTPStatus.NOT_FOUND,
		HTTPStatus.BAD_REQUEST,
		HTTPStatus.UNAUTHORIZED,
		HTTPStatus.CONFLICT,
		HTTPStatus.REQUEST_TIMEOUT,
		HTTPStatus.REQUEST_ENTITY_TOO_LARGE,
		HTTPStatus.INTERNAL_SERVER_ERROR,
		HTTPStatus.NOT_IMPLEMENTED,
		}

	def __init__(self, resp):
		self._resp = resp
		self._body = None
		self._errno = None

	@property
	def status(self):
		return self._resp.status

	@property
	def reason(self):
		return self._resp.reason

	def body(self):
		if self._body == None:
			self._body = self._resp.read()
		return self._body

	def iserror(self):
		if self._resp.status in Response.codes:
			return True
		else:
			return False

	@property
	def errno(self):
		if self._body == None:
			self._body = self._resp.read()
			err_json = self._body.decode('utf8')
			err_data = json.loads(err_json)
			self._errno = err_data.get("rc")

		return self._errno

	def display(self, request):
		if request.method != 'GET':
			return;

		if request.command == 'fs':
			# Parse resp body
			display = "{:<8}{:<36}{:<16}{:<8}\t{}"

			print(display.format("FS ID",
						"FS Name",
						"Exported",
						"Protocol",
						"Export Options"))

			if self._resp.status == HTTPStatus.NO_CONTENT:
				return;

			fs_id = 1
			fs_list = json.loads(self.body().decode("utf-8"))

			for fs in fs_list:
				fs_name = fs.get("fs-name")
				endpoint_options = fs.get("endpoint-options")
				if endpoint_options == None:
					is_exported = "NO"
					protocol = "None"
					endpoint_options = ""
				else:
					is_exported = "YES"
					protocol = endpoint_options.get('proto')

				print(display.format(fs_id,
							fs_name,
							is_exported,
							protocol,
							json.dumps(endpoint_options)))
				fs_id += 1

class HttpRequest(Request):
	"""
	Represents a HTTP Request.
	"""
	request_map = \
		{	'create' : 'PUT',
			'delete' : 'DELETE',
			'list'   : 'GET',
			# Add more commads.
		}

	def __init__(self, command):
		super().__init__(command)

		# Form HTTP Request parameters
		self._url_base = "/" + self.command
		if self.action() not in HttpRequest.request_map.keys():
			raise Exception('invalid command %s', self.action())
		self._method = HttpRequest.request_map[self.action()]

		self._url_path = ""
		self._content = {}
		self._headers = {}

	@property
	def args(self):
		return super().args()

	@property
	def command(self):
		return super().command()

	@property
	def method(self):
		return self._method

	@method.setter
	def method(self, method):
		self._method = method

	@property
	def url_base(self):
		return self._url_base

	@url_base.setter
	def url_base(self, url):
		self._url_base = url

	@property
	def url_path(self):
		return self._url_path

	@url_path.setter
	def url_path(self, path):
		self._url_path = path

	@property
	def content(self):
		return self._content

	@content.setter
	def content(self, content):
		self._content = content

	@property
	def headers(self):
		return self._headers

	@headers.setter
	def headers(self, header):
		self._headers.update(header)

class Client:
	"""
	Represents a generic client.
	"""
	def __init__(self, host, port):
		self._host = host
		self._port = port

	def process(self, request):
		raise Exception("not implemented")
		return 0

class RestClient(Client):
	"""
	Concrete class to communicate with EFS management API.
	"""

	def __init__(self, host, port):
		super().__init__(host, port)

		# Make HTTP connection to server.
		self.server = http.client.HTTPConnection(self._host, self._port)

	def send(self, req):
		try:
			self.server.request(req.method,
					    req.url_base + "/" + req.url_path,
					    req.content,
					    req.headers)
		except Exception as e:
			raise Exception("unable to send request to %s:%s. %s", self._host, self._port, e)

	def recv(self):
		rc = self.server.getresponse();
		resp = Response(rc)
		return resp;

	def compose(self, request):
		self.request = request
		args = request.args

		# Set url_path
		if request.method != "PUT":
			# We send request params of non PUT request as url path params
			if len(args) != 0:
				request.url_path = args[0]
		else:
			# Set content
			content = {}
			options = None
			argc = len(args)
			content["name"] = args[0]
			cmds_with_options = { 'fs', 'endpoint' }

			if request.command in cmds_with_options:
				if argc > 1:
					options = args[1]
			else:
				options = None

			if options != None:
				content["options"] = {}
				option_list = options.split(',')
				for option_token in option_list:
					option = option_token.split('=')
					key = option[0]
					val = option[1]
					content["options"].update({key : val});

			# Add more option parameter's here.
			content = json.dumps(content).encode('utf-8')
			request.content = content

		# Set headers
		if bool(request.content):
			# There is content for the http request.
			# Set the headers.
			header = {"Content-Type" : "application/json"}
			request.headers.update(header)
			header = {"Content-Length" : str(len(request.content))}
			request.headers.update(header)
		else:
			header = {"Content-Length" : "0"}
			request.headers.update(header)

	def process(self, request):
		"""
		Call remote API method synchronously.
		"""

		# Compose request
		self.compose(request)

		# Send the request
		resp = None
		self.send(request)
		# Get response
		resp = self.recv()
		return resp;

def main(argv):
	"""
	Parse and execute command line to obtain command structure.
	Execute the CLI
	"""
	if len(sys.argv) < 2:
		print ("Incorrect usage, please use -h or --help for usage")
		return -1
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
		if resp.iserror():
			print(resp.reason)
			rc = resp.errno
		else:
			rc = 0
			print(resp.reason)
			resp.display(request)
		return rc
	except Exception as exception:
		sys.stderr.write('%s\n' %exception)
		return 1

if __name__ == '__main__':
	sys.exit(main(sys.argv))
