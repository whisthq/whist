#!/usr/bin/env python
import psycopg2
import subprocess
from functools import wraps
from contextlib import contextmanager
from errors import catch_process_error

# connection = psycopg2.connect(user="postgres",
#                               # password="pyantive@#29",
#                               host="127.0.0.1",
#                               port="8888",
#                               database="postgres")

# cursor = connection.cursor()


# cursor.execute("select schema_name from information_schema.schemata")
# record = cursor.fetchall()

DB_CONNECTION_HOST = "localhost"
DB_DEFAULT_USER = "postgres"

def with_host_user(func):
    @wraps(func)
    def with_host_user_wrapper(port, *args, **kwargs):
        return func(DB_CONNECTION_HOST,
                    DB_DEFAULT_USER,
                    str(port),
                    *args,
                    **kwargs)
    return with_host_user_wrapper

@catch_process_error
@with_host_user
def sql_commands(host, user, port, sql_file_path):
    subprocess.run(["psql",
                    "--file", sql_file_path,
                    "--host", host,
                    "--username", user,
                    "--port", str(port)],
                   check=True,
                   stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE)

@with_host_user
def is_ready(host, user, port):
    completed = subprocess.run(["pg_isready",
                                "--host", host,
                                "--username", user,
                                "--port", str(port)],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)

    return completed.returncode == 0
