#!/usr/bin/env python
import subprocess
import sys
from errors import catch_process_error, catch_value_error


@catch_value_error
@catch_process_error
def dump_schema(url):
    """Retrieve the schema of a running database.

    Given a database configuration, returns a string of SQL statements
    representing the schema of a running database.

    Uses the PostGreSQL pg_dump tool to generate the statements.

    Args:
        url: A string representing a PostgreSQL connection URL.
    Returns:
        A string of the SQL commands that represent the schema of the
        database.
    """
    completed = subprocess.run(
        ["pg_dump", "--no-owner", "--no-privileges", "--schema-only", url],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
        text=True
    )
    return completed.stdout

if __name__ == "__main__":

    _, url = sys.argv

    print(dump_schema(url))
