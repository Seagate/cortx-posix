'''
Created on Sep 7, 2020

@author: 730728
'''
from http import HTTPStatus
import json

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
if __name__ == '__main__':
    pass