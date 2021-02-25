#!/usr/bin/env python
import tempfile
import os
from pathlib import Path

# How long we'll wait for the Docker container + PostgreSQL db to be ready.
TIMEOUT_SECONDS = 10

# The base url of the heroku API.
HEROKU_BASE_URL = "https://api.heroku.com"

# The name of the app as recorded in Heroku,
# for example: fractal-dev-server.
# HEROKU_APP_NAME = os.environ["HEROKU_APP_NAME"]

# The name of the Heroku config variable that contains the
# URL to connect to the database.
HEROKU_DB_URL_KEY = "DATABASE_URL"

# The path to the "merging" schema. This is the .sql file that contains
# the commands representing the schema of the database from the perspective
# of the feature branch that is being merged in.
# It is the "new source of truth".
# SCHEMA_PATH_MERGING = os.environ["SCHEMA_PATH_MERGING"]

# The path to the "current" schema. This is the .sql file that contains
# the commands representing the schema of the dev/staging/prod database.
# It is the "old source of truth".
#
# The difference between the "merging"
# schema and this "current" schema will be applied to the dev/staging/prod
# database to perform the migration.
#
# This is set to a tempfile, which will be written to with the schema from
# the live dev/staging/prod database.
SCHEMA_PATH_CURRENT = Path(tempfile.gettempdir()).joinpath("tempdb.sql")

# This temporary file will be written to with the "diff" of the "merging"
# schema and the "current" schema. It needs to be written to a file so it
# can be passed to a PostgreSQL tool for verification.
SCHEMA_PATH_DIFFING = Path(tempfile.gettempdir()).joinpath("diffdb.sql")

# These values are the default values for a new PostgreSQL database
# using the postgres Docker container. They are here as config vars
# so we can be explicit about what we are passing to functions, and so
# errors are more easily traced.
POSTGRES_DEFAULT_HOST = "localhost"
POSTGRES_DEFAULT_IMAGE = "postgres"
POSTGRES_DEFAULT_USERNAME = "postgres"
POSTGRES_DEFAULT_DBNAME = "postgres"
POSTGRES_DEFAULT_PASSWORD = "yKCmXHwMIKVF8rLgry9e"
POSTGRES_DEFAULT_PORT = 5432

# These values are dictionaries that represent the configuration for the
# databases that need to be initialized to perform the schema diff.
# The database connection functions used for the migration are set up
# to directly accept these dictionaries to create a connection
DB_CONFIG_DEFAULT = {
    "host": POSTGRES_DEFAULT_HOST,
    "port": POSTGRES_DEFAULT_PORT,
    "username": POSTGRES_DEFAULT_USERNAME,
    "password": POSTGRES_DEFAULT_PASSWORD,
    "dbname": POSTGRES_DEFAULT_USERNAME,
}
DB_CONFIG_MERGING = {**DB_CONFIG_DEFAULT, "port": 9945}
DB_CONFIG_CURRENT = {**DB_CONFIG_DEFAULT, "port": 9955}

# These values contain collections of statments that should be ignored in the
# generated diff for the migration progress. They are statements that are not
# "compatible" with migra, in the sense that they will always appear in a diff
# even if the tables/columns affected are actually unchanged.
#
# Based on the the statements themselves it is easy to see that they are all
# commands that involve random UUIDs.
# Migra is set up to "take no reponsibility" for schema statements that are
# not deterministic, and will place them in the diff every time.
#
# As long as we have randomness involved in our schema design, we'll need to
# manually "ignore" the statements that use it.
IGNORE_SQL_STATEMENTS_HEAD = {'alter extension "pg_stat_statements" update to'}
IGNORE_SQL_STATEMENTS_COMPLETE = {
    'alter table "hdb_catalog"."event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."event_log" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_action_log" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_cron_event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_cron_events" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_scheduled_event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_scheduled_events" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_version" alter column "hasura_uuid" set default public.gen_random_uuid();',
}

PERFORM_DATABASE_MIGRATION = os.environ.get("PERFORM_DATABASE_MIGRATION")
