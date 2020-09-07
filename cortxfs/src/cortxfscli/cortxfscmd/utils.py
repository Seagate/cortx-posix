'''
Created on Sep 7, 2020

@author: 730728
'''
import re
import json

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


if __name__ == '__main__':
    pass