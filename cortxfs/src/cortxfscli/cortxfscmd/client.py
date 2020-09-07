'''
Created on Sep 7, 2020

@author: 730728
'''


from response import Response
import json
import http
from http import HTTPStatus
import http.client

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
    Concrete class to communicate with CORTXFS management API.
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
            cmds_with_options = { 'fs', 'endpoint', 'auth' }

            if request.command == 'auth':
                content["type"] = args[0]
                if args[0] == "ldap":
                    content["server"] = args[1]
                    content["baseDN"] = args[2]
                    content["admin_account"] = args[3]
                    content["admin_account_pw"] = args[4]
                else:
                    print("Unsupported auth type")
                    return
            else:
                content["name"] = args[0]
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