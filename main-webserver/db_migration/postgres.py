#!/usr/bin/env python
import subprocess
from functools import wraps
from contextlib import contextmanager
from errors import catch_process_error, catch_value_error
import config


def postgres_parse_url(db_config):
    """Formats a PostgreSQL URL string given a configuration dictionary.

    Args:
        db_config: A dictionary of database configuration keys, including
        "host", "port", "dbname", "username", "password".
    Returns:
        A URL string formatted with the values in db_config.
    """
    if db_config.get("url"):
        return db_config["url"]

    host = db_config["host"]
    port = db_config["port"]
    dbname = db_config["dbname"]
    username = db_config["username"]
    password = db_config["password"]

    return f"postgresql://{username}:{password}@{host}:{port}/{dbname}"


def with_postgres_url(func):
    """A decorator to transform the arguments to PostgreSQL functions.

    The function signatures of PostgreSQL commands are inconsistent, so
    this function provides a common interface for database configuration
    to the rest of the applicaton. It passes the result along as a formatted
    URL to the decorated PostgreSQL function. It accepts keyword parameters:

    host= a string
    port= a number or string
    dbname= a string
    username= a string
    password= a string

    Args:
        func: A function that accepts a string URL as its first argument.
    Returns:
        The input function, partially applied with a PostgreSQL URL as its
        first argument.
    """

    @wraps(func)
    def with_postgres_url_wrapper(*args, **kwargs):
        try:
            return func(postgres_parse_url(kwargs), *args)
        except KeyError:
            raise ValueError(
                (
                    "Database connection needs "
                    + "host, port, username, password, and dbname keywords."
                ),
                kwargs,
            ) from None

    return with_postgres_url_wrapper


@catch_process_error
@with_postgres_url
def sql_commands(url, sql_file_path):
    """Runs a .sql file of commands against a database.

    Given a .sql filepath and a database configuration, the .sql file
    is read as a single string. The entire string is executed in a single
    transaction against the database, which must already be running.

    Args:
        url: A string representing a PostgreSQL connection URL.
        sql_file_path: A string representing a path to a .sql file.
    Returns:
        None
    """
    subprocess.run(
        ["psql", "--single-transaction", "--file", str(sql_file_path), url],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


# Not using with_postgres_url decorator,
# as pg_isready does not accept a url as a parameter.
@catch_value_error
@catch_process_error
def is_ready(host=None, port=None, username=None, **kwargs):
    """Check if a database is ready to accept connections.

    Given a database configuration, return True or False based
    on whether the database is ready to connect.

    Uses the PostgreSQL tool pg_isready to ping the database.

    Args:
        kwargs: A dictionary representing database configuration.
    Returns:
        True if the database is ready for connections, False otherwise.
    Raises:
        CalledProcessError: Raised if the return code of the completed process
                            signals an error.
    """
    try:
        completed = subprocess.run(
            [
                "pg_isready",
                "--host",
                host,
                "--port",
                str(port),
                "--username",
                username,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
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
def schema_diff_from_urls(db_config_A, db_config_B):
    """Generate a schema diff between two running databases

       Given two database configurations, run the `migra` tool against them
       to generate a diff of their schemas in the form of a string of SQL
    {"title": "There's some changes to be made to the schema!",
               "body": "Running the SQL commands below will perform the migration.",

               "sql":  ""}   statements required to migrate db_A to db_B.

       migra is a little peculiar with exit codes:
       0 is a successful run, producing no diff (identical schemas)
       1 is a error
       2 is a successful run, producing a diff (non-identical schemas)
       3 is a successful run, but producing no diff, meaning the diff is "unsafe"

       An "unsafe" diff produces destructive SQL statements, and requires Migra
       to be called with the "--unsafe" flag.

       Args:
           db_config_A: A dictionary representing a database configuration.
           db_config_B: A dictionary representing a database configuration.
       Returns:
          A string representing the diff between two schemas, or None if no diff.
    """
    identity = lambda x: x
    url_A = with_postgres_url(identity)(**db_config_A)
    url_B = with_postgres_url(identity)(**db_config_B)

    # We call subprocess with the "check" keyword here, which will raise
    # an error if the exit code is not zero. We have some processing to do
    # on the error object, so we catch it.
    #
    # We use "check" purposefully, because if we don't get an exit code that
    # we expect, then the subprocess.CalledProcessError will bubble up with
    # its normal traceback and data.
    try:
        completed = subprocess.run(
            ["migra", url_A, url_B],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )

        return 0, None

    except subprocess.CalledProcessError as e:
        if e.returncode == 2:
            return 2, e.stdout.decode("utf-8")
        if e.returncode == 3:
            try:
                unsafe_completed = subprocess.run(
                    ["migra", "--unsafe", url_A, url_B],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=True,
                )
            except subprocess.CalledProcessError as e:
                if e.returncode == 2:
                    return 3, e.stdout.decode("utf-8")
                raise e
        raise e
