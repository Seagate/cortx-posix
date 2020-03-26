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
		HTTPStatus.CONFLICT,
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

	@property
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
			err_json = self._body.decode('utf8').replace("'", '"')
			err_data = json.loads(err_json)
			self._errno = err_data.get("rc")

		return self._errno

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
			content["name"] = args[0]
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
			if request.command == 'fs' and request.method == 'GET':
				# Parse resp body
				print(resp.body)
		return rc
	except Exception as exception:
		sys.stderr.write('%s\n' %exception)
		return 1

if __name__ == '__main__':
	sys.exit(main(sys.argv))
