#!/usr/bin/env python
import subprocess
from functools import wraps
from contextlib import contextmanager
from errors import catch_process_error, catch_value_error
import config

def postgres_parse_url(config):
    """Formats a URL string given a configuration dictionary"""
    if config.get("url"):
        return config["url"]

    host = config["host"]
    port = config["port"]
    dbname = config["dbname"]
    username = config["username"]
    password = config["password"]

    return f"postgres://{username}:{password}@{host}:{port}/{dbname}"

def with_postgres_url(func):
    """A decorator to transform the arguments to PostgreSQL functions

    The function signatuers of PostgreSQL commands are inconsistent, so
    this function provides a common interface for database configuration
    to the rest of the applicaton. It passes the result along as a formatted
    URL to the decorated PostgreSQL function. It accepts keyword parameters:

    host= a string
    port= a number or string 
    dbname= a string
    username= a string
    password= a string
    """
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
    """Runs a .sql file of commands against a database

    Given an .sql filepath and a database configuration, the .sql file
    is read as a single string. The entire string is executed in a single
    transaction against the database, which must already be running.
    """
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
    """Check if a database is ready to accept connections

    Given a database configuration, return True or False based
    on whether the database is ready to connect.

    Uses the PostgreSQL tool pg_isready to ping the database.
    """
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
    """Retrieve the schema of a running database

    Given a database configuration, returns a string of SQL statements
    representing the schema of a running database.

    Uses the PostGreSQL pg_dump tool to generate the statements.
    """
    completed = subprocess.run(["pg_dump",
                                "--no-owner",
                                "--no-privileges",
                                "--schema-only",
                                *args],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               check=True)
    return completed.stdout.decode("utf-8")


@catch_value_error
@catch_process_error
def schema_diff(db_config_A, db_config_B):
    """Generate a schema diff between two running databases

    Given two database configurations, run the `migra` tool against them
    to generate a diff of their schemas in the form of a string of SQL
    statements required to migrate db_A to db_B.

    migra is a little peculiar with exit codes:
    0 is a successful run, producing no diff (identical schemas)
    1 is a error
    2 is a successful run, producing a diff (non-identical schemas)
    3 is a successful run, but producing no diff, meaning the diff is "unsafe"

    An "unsafe" diff produces destructive SQL statements, and requires Migra
    to be called with the "--unsafe" flag.
    """
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
