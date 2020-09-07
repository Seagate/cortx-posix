'''
Created on Sep 7, 2020

@author: 730728
'''

cortxfscli_validation_rule_dev='./cortxfscli_validation_rules.json'
# Don't allow any special chars for FS names, except '-', '_'
fs_name_regex="[^A-Za-z0-9/]"
fs_name_max_len=255
cortxfscli_default_validation_rules = """{
    "nfs" : {
            "proto" : {"str": "nfs"},
            "status" : {"set" : "enabled,disabled"},
            "secType" : {"set" : "none,sys"},
            "Filesystem_id" : {"regex" : "[^0-9+.0-9+$]", "limit" : "100"},
            "client" : {"max_count" : "10"},
            "clients" : {"regex" : "[^A-Za-z0-9.*/]", "limit" : "100"},
            "Squash" : {"set" : "no_root_squash,root_squash"},
            "access_type" : {"set" : "RW,R,W,None"},
            "protocols" : {"set" : "4,4.1"},
            "pnfs_enabled" : {"set" : "true,false"},
            "data_server" : {"regex" : "[^A-Za-z0-9.*/]", "limit" : "16"}
    },

    "smb" : {
            "proto" : {"str" : "smb"},
            "status" : {"set" : "unsupported"}
    }
}"""