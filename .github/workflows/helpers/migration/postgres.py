#!/usr/bin/env python
import subprocess
from functools import wraps
from contextlib import contextmanager
from errors import catch_process_error, catch_value_error
import config

def postgres_parse_url(config):
    if config.get("url"):
        return config["url"]

    host = config["host"]
    port = config["port"]
    dbname = config["dbname"]
    username = config["username"]
    password = config["password"]

    return f"postgres://{username}:{password}@{host}:{port}/{dbname}"

def with_postgres_url(func):
    @wraps(func)
    def with_postgres_url_wrapper(*args, **kwargs):
        try:
            return func(*args, postgres_parse_url(kwargs))
        except KeyError:
            raise ValueError(
                ("Database connection needs "
                 + "host, port, username, password, and dbname keywords."),
                kwargs) from None

    return with_postgres_url_wrapper


@catch_process_error
@with_postgres_url
def sql_commands(sql_file_path, *args):
    subprocess.run(["psql",
                    "--single-transaction",
                    "--file", str(sql_file_path),
                    *args],
                   check=True,
                   stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE)


# Not using with_postgres_url decorator,
# as pg_isready does not accept a url as a parameter.
@catch_value_error
@catch_process_error
def is_ready(host=None, port=None, username=None, **kwargs):
    try:
        completed = subprocess.run(["pg_isready",
                                    "--host", host,
                                    "--port", str(port),
                                    "--username", username],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   check=True)
        return True
    # Return code "1" means that the call was successful and db isn't ready
    # Return code "2" means that the call received no reponse
    # Other return codes are errors
    except subprocess.CalledProcessError as e:
        if (e.returncode == 1) or (e.returncode == 2):
            return False
        else:
            raise e


@catch_value_error
@catch_process_error
@with_postgres_url
def dump_schema(*args):
    completed = subprocess.run(["pg_dump",
                                "--no-owner",
                                "--no-privileges",
                                "--schema-only",
                                *args],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               check=True)
    return completed.stdout.decode("utf-8")


# Migra is a little peculiar with exit codes
# 0 is a successful run, producing no diff (identical schemas)
# 1 is a error
# 2 is a successful run, producing a diff (non-identical schemas)
# 3 is a successful run, but producing no diff, meaning the diff is "unsafe"
#
# An "unsafe" diff produces destructive SQL statements, and requires Migra
# to be called with the "--unsafe" flag.
@catch_value_error
@catch_process_error
def schema_diff(db_config_A, db_config_B):
    identity = lambda x: x
    url_A = with_postgres_url(identity)(**db_config_A)
    url_B = with_postgres_url(identity)(**db_config_B)

    try:
        completed = subprocess.run(["migra",
                                    "--unsafe",
                                    url_A,
                                    url_B],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   check=True)
    except Exception as e:
        if e.returncode == 0:
            return None
        if e.returncode == 2:
            return e.stdout.decode("utf-8")
        raise e

