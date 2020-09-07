'''
Created on Sep 7, 2020

@author: 730728
'''
from utils import *
from constants import *

class Command(object):
    """
    Base class for all commands supported by CORTXFS CLI
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
            default=cortxfscli_validation_rule_dev)
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
            1. If local dev version exists, i.e. cortxfscli_validation_rule_dev 
            2. If user passed an existing file
            3. Embedded
        ''' 
        if os.path.isfile(cortxfscli_validation_rule_dev) == True:
            print ("Using dev validation rules")
            cortxfscli_config_rules = read_conf_file(cortxfscli_validation_rule_dev)
        elif os.path.isfile(args.config_validation) == True:
            print ("Using external validation rules")
            cortxfscli_config_rules = read_conf_file(args.config_validation)
        else:
            print ("Using embedded validation rules")
            cortxfscli_config_rules = json.loads(cortxfscli_default_validation_rules)

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

            validate_inp_config_params(cortxfscli_config_rules, args.args[1])


class AuthCommand(Command):
    """
    Contains functionality to handle Auth Setup commands.
    """

    def __init__(self, args):
        super().__init__(args)

    def name(self):
        return "auth"

    @staticmethod
    def add_args(parser):
        sbparser = parser.add_parser("auth", help='setup, show, check or remove Auth Setup.')
        sbparser.add_argument('action', help='action', choices=['setup', 'show', 'check', 'remove'])
        sbparser.add_argument('args', nargs='*', default=[], help='Auth Setup command options')
        sbparser.set_defaults(command=AuthCommand)

    def validate_args_payload(self, args):
        if args.action.lower() == 'setup':
            # Below check is based on current minimum arguments
            # needed for ldap. Later after adding other options,
            # this minimum value will change.
            if len(args.args) < 5:
                throw_exception_with_msg("Less args for " + \
                args.action.lower())
            if args.args[0] == "ldap":
                # Need to add checks for arguments provided.
                if len(args.args)!= 5:
                    throw_exception_with_msg("Incorrect args for " + \
                    args.action.lower() + " of type " + \
                    args.args[0])
                # TODO: Add validation checks
            else:
                throw_exception_with_msg("Incorrect type for " + \
                args.action.lower())


class CommandFactory(object):
    """
    Factory for representing and creating command objects
    using a generic skeleton.
    """

    commands = {FsCommand, EndpointCommand, AuthCommand}

    def get_command(argv):
        """
        Parse the command line as per the syntax and return command.
        """

        parser = argparse.ArgumentParser(description='CORTXFS CLI command')
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


if __name__ == '__main__':
    pass