'''
Created on Sep 7, 2020

@author: 730728
'''


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


class HttpRequest(Request):
    """
    Represents a HTTP Request.
    """
    request_map = \
        {    'create' : 'PUT',
            'delete' : 'DELETE',
            'list'   : 'GET',
            # Auth commads.
            'setup'   : 'PUT',
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


if __name__ == '__main__':
    pass