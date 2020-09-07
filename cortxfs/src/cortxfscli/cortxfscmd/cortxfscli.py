'''
Created on Sep 7, 2020

@author: 730728
'''
import sys
import os

from cortxfscmd.command import CommandFactory
from cortxfscmd.request import HttpRequest
from cortxfscmd.client  import RestClient

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