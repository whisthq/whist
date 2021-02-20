#!/usr/bin/env python
import os
import containers
import sys
import tempfile
import output
from postgres import sql_commands, schema_diff
from pprint import pprint
from pathlib import Path
from config import DB_CONFIG_CURRENT, DB_CONFIG_MERGING


def db_from_schema(schema_path, **kwargs):
    """Creates a Docker container, configured with a PostgreSQL database.

    Given a path to a file containing `.sql` commands, and kwargs representing
    configuration for a database connection, spin up a Docker container with
    a PostgreSQL database, wait for the database to initialize, and execute
    the SQL commands on the database.

    Args:
        schema_path: A string, representing a path to a .sql file.
        kwargs: Configuration keywords for the database connection,
                including "host", "port", "username", "password", and "dbname".
    Returns:
        A Docker container object with the initialized database..

    """
    container = containers.run_postgres_container(**kwargs)
    sql_commands(schema_path, **kwargs)
    return container


def write_to_file(path, string):
    """A helper function to write a string to a file.

    Args:
        path: A string representing a path to a writeable file.
        string: A string to be written to the file at "path".
    Returns:
        None
    """
    with open(path, "w") as f:
        f.write(string)


if __name__ == "__main__":
    _, PATH_CURRENT, PATH_MERGING = sys.argv
    PATH_VALIDATE = tempfile.NamedTemporaryFile()

    # Dump the schema from the "current" database (fractal server)
    # write_to_file(SCHEMA_PATH_CURRENT, postgres.dump_schema(url=DB_URL))

    # Load the current .sql schema into a fresh postgres database
    current = db_from_schema(PATH_CURRENT, **DB_CONFIG_CURRENT)

    # Load the merging .sql schema into a fresh postgres database
    merging = db_from_schema(PATH_MERGING, **DB_CONFIG_MERGING)

    # Generate a diff of sql commands between merging and current
    code, diff = schema_diff(DB_CONFIG_CURRENT, DB_CONFIG_MERGING)

    # The exit codes for this script will mimic the migra diff tool
    # that it depends on.
    #
    # migra uses the following exit codes:
    # 0 is a successful run, producing no diff (identical schemas)
    # 1 is a error
    # 2 is a successful run, producing a diff (non-identical schemas)
    # 3 is a successful run, but producing no diff, meaning the diff is "unsafe"
    #
    # We'll introduce our own exit codes to mark certain issues:
    # 4 is a successful run, but producing a diff that does not result in
    #   identical databases upon application.

    if code not in {0, 2, 3, 4}:
        raise Exception(f"Received an unexpected return code '{code}' from "
                        + "postgres diff tool 'migra'.")

    # If there's no schema diff, exit with code 0.
    if code == 0:
        sys.exit(0)

    # If there's a diff, we need to test it against the current database
    # Create a temporary file for the diff we just generated, so that
    # it can be run as a SQL command script against the current database
    write_to_file(PATH_VALIDATE.name, diff)
    sql_commands(PATH_VALIDATE.name, **DB_CONFIG_CURRENT)

    # A successful diff between current and merging should render
    # current and merging when the diff is applied
    #
    # Now that we have "applied" the diff to the current database,
    # we run generate a diff once more in hopes of receiving nothing back
    # If this new diff is empty, then current and merging are identical
    # and we can expect the migration to be successful
    _, verification_diff = schema_diff(DB_CONFIG_CURRENT, DB_CONFIG_MERGING)

    if not verification_diff:
        # The verification_diff is empty, so the two databases are identical.
        # This indicates the migration will go smoothly.
        if code == 2:
            print(diff)
            sys.exit(2)
            # sys.exit(0)
        if code == 3:
            print(diff)
            sys.exit(3)
            # sys.exit(0)

    else:
        # The diff will not produce an identical database schema.
        # Output the SQL commands that will not migrate properly.
        print(verification_diff)
        sys.exit(4)
        # sys.exit(0)
