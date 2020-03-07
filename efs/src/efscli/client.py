#!/usr/bin/python3

#
# Filename:         client.py
# Description:      REST client and request utilities for efs-cli.
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

import http.client
import json

class Request:
	"""
	Represents a request which is processed by a client.
	"""

	def __init__(self, command):
		self._command = command

	def type(self):
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
	def __init__(self, status, reason):
		self.status = status
		self.reason = reason
		self.body = {}

	def body(self):
		self.body

	def set_body(self, body):
		self.body = body

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
		self._url_base = "/" + self.type()
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
		resp = Response(rc.status, rc.reason)
		if self.request.method == 'GET' and rc.status != 204:
			resp.body = rc.read()
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
