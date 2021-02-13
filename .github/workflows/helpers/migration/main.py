#!/usr/bin/env python
import os
import pretty_errors
import containers
import heroku
import postgres
import sys
import tempfile
import output
from pprint import pprint
from pathlib import Path
from config import (HEROKU_APP_NAME,
                    HEROKU_DB_URL_KEY,
                    SCHEMA_PATH_CURRENT,
                    SCHEMA_PATH_MERGING,
                    SCHEMA_PATH_DIFFING,
                    DB_CONFIG_MERGING,
                    DB_CONFIG_CURRENT,
                    PERFORM_DATABASE_MIGRATION)


def db_from_schema(schema_path, **kwargs):
    container = containers.run_postgres_container(**kwargs)
    postgres.sql_commands(schema_path, **kwargs)
    return container


def write_to_file(path, string):
    with open(path, "w") as f:
        f.write(string)

# Dump the schema from the "current" database (fractal server)
# using Heroku credentials, and save to a temporary file
heroku_config = heroku.config_vars(HEROKU_APP_NAME)
heroku_db_url = heroku_config[HEROKU_DB_URL_KEY]

write_to_file(SCHEMA_PATH_CURRENT, postgres.dump_schema(url=heroku_db_url))

# Load the merging .sql schema into a fresh postgres database
merging = db_from_schema(SCHEMA_PATH_MERGING, **DB_CONFIG_MERGING)

# Load the current .sql schema into a fresh postgres database
current = db_from_schema(SCHEMA_PATH_CURRENT, **DB_CONFIG_CURRENT)

# Generate a diff of sql commands between merging and current
diff = postgres.schema_diff(DB_CONFIG_CURRENT, DB_CONFIG_MERGING)

# If there's no schema diff, exit with code 0.
if not diff:
    output.title("No schema changes for this PR!")
    output.body("No database migration will be performed.")

# If there's a diff, we need to test it against the current database
# Create a temporary file for the diff we just generated, so that
# it can be run as a SQL command script against the current database
write_to_file(SCHEMA_PATH_DIFFING, diff)
postgres.sql_commands(SCHEMA_PATH_DIFFING, **DB_CONFIG_CURRENT)

# A successful diff between current and merging should render
# current and merging when the diff is applied
#
# Now that we have "applied" the diff to the current database,
# we run generate a diff once more in hopes of receiving nothing back
# If this new diff is empty, then current and merging are identical
# and we can expect the migration to be successful
test_diff = postgres.schema_diff(DB_CONFIG_CURRENT, DB_CONFIG_MERGING)

if not test_diff:
    if PERFORM_DATABASE_MIGRATION:
        # Actually perform migration
        # postgres.sql_commands(SCHEMA_PATH_DIFFING,
        #                       url=heroku_db_url)

        output.title("Database migration performed!")
        output.body(f"The follow SQL commands were sent to {HEROKU_APP_NAME}.")
        output.sql(diff)
    else:
        output.title("There's some changes to be made to the schema!")
        output.body("Running the SQL commands below will perform the migration.")
        output.sql(diff)
else:
    output.alert("This migration might not go properly.")
    output.body("Here's the diff between schemas:")
    output.sql(diff)
    output.sep()
    print("Here's what didn't make it through the diff test:")
    ouput.sql(test_diff)
