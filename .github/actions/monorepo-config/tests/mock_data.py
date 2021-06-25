#!/usr/bin/env python
import yaml
import json


example_profiles = ["dev", "prod"]

example_nested_map = {
    "API_KEY": {
        "dev": {"x": "t", "y": "u", "z": "v"},
        "prod": {"d": "q", "e": "r", "f": "s"},
    },
    "WEB_URL": {
        "dev": {"x": "t", "y": "u", "z": "v"},
        "prod": {"d": "q", "e": "r", "f": "s"},
    },
    "DEV_NUM": {"dev": "twenty", "prod": "thirty"},
}

# Below we see some "fs schemas" that are suitable to pass to
# "utils.temporary_fs". A temporary file system will be created
# using the files and folders described in this data structure.
#
# If a key has a dictionary has its value, then it is a folder.
# Otherwise it is a file, and file with a key name will be created.
#
# A file key's value will be written to as the contents of the file.
# If the value is type(bytes), it will be written as bytes with
# mode "wb". Otherwise, it will use mode "r" and write to the data
# to the file as a string.


# These file system should parse successfully into a configuration
# object. "profiles.yml" exists, and the "schema" folder containers
# only ".yml" files. All the schema files only make use of profiles
# that are described in "profiles.yml".
config_simple_fs = {
    "config": {
        "profiles.yml": yaml.dump(["dev", "stg", "prd"]),
        "schema": {
            "web.yml": yaml.dump(
                {
                    "URL": {
                        "dev": "http://url-dev.com",
                        "stg": "http://url-stg.com",
                        "prd": "http://url-prd.com",
                    },
                    "WEB": {
                        "dev": "http://web-dev.com",
                        "stg": "http://web-stg.com",
                        "prd": "http://web-prd.com",
                    },
                    "COM": {
                        "dev": "http://com-dev.com",
                        "stg": "http://com-stg.com",
                        "prd": "http://com-prd.com",
                    },
                }
            ),
            "auth.yml": yaml.dump(
                {"SERVER": "102.347.188", "TOKEN": "tokenla98hddjhh2jjd"}
            ),
            "secret.yml": yaml.dump(
                {
                    "KEY": {
                        "dev": "key01000",
                        "stg": "key02000",
                        "prd": "key03000",
                    },
                    "API": {
                        "dev": "api01000",
                        "stg": "api02000",
                        "prd": "api03000",
                    },
                }
            ),
        },
    },
}

# These are some expected results for the output of "parse",
# given different profile= arguments.


config_simple_json_default = {
    "URL": {
        "dev": "http://url-dev.com",
        "stg": "http://url-stg.com",
        "prd": "http://url-prd.com",
    },
    "WEB": {
        "dev": "http://web-dev.com",
        "stg": "http://web-stg.com",
        "prd": "http://web-prd.com",
    },
    "COM": {
        "dev": "http://com-dev.com",
        "stg": "http://com-stg.com",
        "prd": "http://com-prd.com",
    },
    "SERVER": "102.347.188",
    "TOKEN": "tokenla98hddjhh2jjd",
    "KEY": {
        "dev": "key01000",
        "stg": "key02000",
        "prd": "key03000",
    },
    "API": {
        "dev": "api01000",
        "stg": "api02000",
        "prd": "api03000",
    },
}

config_simple_json_dev = {
    "URL": "http://url-dev.com",
    "WEB": "http://web-dev.com",
    "COM": "http://com-dev.com",
    "SERVER": "102.347.188",
    "TOKEN": "tokenla98hddjhh2jjd",
    "KEY": "key01000",
    "API": "api01000",
}


config_simple_json_stg = {
    "URL": "http://url-stg.com",
    "WEB": "http://web-stg.com",
    "COM": "http://com-stg.com",
    "SERVER": "102.347.188",
    "TOKEN": "tokenla98hddjhh2jjd",
    "KEY": "key02000",
    "API": "api02000",
}


config_simple_json_prd = {
    "URL": "http://url-prd.com",
    "WEB": "http://web-prd.com",
    "COM": "http://com-prd.com",
    "SERVER": "102.347.188",
    "TOKEN": "tokenla98hddjhh2jjd",
    "KEY": "key03000",
    "API": "api03000",
}


# In this config file system, someone messed up the profiles in secrets.yml.
# They're invalid, and we should expect our validation system to throw an
# error.
config_fs_bad_profiles = {
    "config": {
        "profiles.yml": yaml.dump(["dev", "stg", "prd"]),
        "schema": {
            "web.yml": yaml.dump(
                {
                    "URL": {
                        "dev": "http://url-dev.com",
                        "stg": "http://url-stg.com",
                        "prd": "http://url-prd.com",
                    },
                    "WEB": {
                        "dev": "http://web-dev.com",
                        "stg": "http://web-stg.com",
                        "prd": "http://web-prd.com",
                    },
                    "COM": {
                        "dev": "http://com-dev.com",
                        "stg": "http://com-stg.com",
                        "prd": "http://com-prd.com",
                    },
                }
            ),
            "auth.yml": yaml.dump(
                {"SERVER": "102.347.188", "TOKEN": "tokenla98hddjhh2jjd"}
            ),
            "secret.yml": yaml.dump(
                {
                    "KEY": {
                        "pro1": "key01000",
                        "pro2": "key02000",
                        "pro3": "key03000",
                    },
                    "API": {
                        "pro1": "api01000",
                        "pro2": "api02000",
                        "pro3": "key03000",
                    },
                }
            ),
        },
    },
}


config_secrets_fs = {
    "config": {
        "profiles.yml": yaml.dump(["dev", "stg", "prd"]),
        "schema": {
            "auth.yml": yaml.dump(
                {"SERVER": "102.347.188", "TOKEN": "tokenla98hddjhh2jjd"}
            ),
            "secret.yml": yaml.dump(
                {
                    "KEY": None,
                    "API": None,
                }
            ),
        },
    },
}

config_secrets_json = {
    "SERVER": "102.347.188",
    "TOKEN": "tokenla98hddjhh2jjd",
    "KEY": "secretKEY",
    "API": "secretAPI",
}

config_secrets_profiles_fs = {
    "config": {
        "profiles.yml": yaml.dump(["dev", "stg", "prd"]),
        "schema": {
            "web.yml": yaml.dump(
                {
                    "URL": {
                        "dev": "http://url-dev.com",
                        "stg": "http://url-stg.com",
                        "prd": "http://url-prd.com",
                    },
                }
            ),
            "secret.yml": yaml.dump(
                {
                    "KEY": {
                        "dev": "key01000",
                        "stg": "key02000",
                        "prd": "key03000",
                    },
                    "API": {
                        "dev": "api01000",
                        "stg": "api02000",
                        "prd": "api03000",
                    },
                }
            ),
        },
    },
}


config_secrets_profiles_json_dev = {
    "URL": "http://url-dev.com",
    "KEY": "secretKEY",
    "API": "secretAPI",
}


config_secrets_profiles_json_stg = {
    "URL": "http://url-stg.com",
    "KEY": "secretKEY",
    "API": "secretAPI",
}

config_secrets_profiles_json_prd = {
    "URL": "http://url-prd.com",
    "KEY": "secretKEY",
    "API": "secretAPI",
}
