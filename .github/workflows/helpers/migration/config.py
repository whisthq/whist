#!/usr/bin/env python
import tempfile
import os
from pathlib import Path

TIMEOUT_SECONDS = 10

HEROKU_BASE_URL = "https://api.heroku.com"

HEROKU_APP_NAME = os.environ["HEROKU_APP_NAME"]

HEROKU_DB_URL_KEY = "DATABASE_URL"

POSTGRES_DEFAULT_HOST = "localhost"

POSTGRES_DEFAULT_IMAGE = "postgres"

POSTGRES_DEFAULT_USERNAME = "postgres"

POSTGRES_DEFAULT_DBNAME = "postgres"

POSTGRES_DEFAULT_PASSWORD = "password"

POSTGRES_DEFAULT_PORT = 5432

SCHEMA_PATH_MERGING = os.environ["SCHEMA_PATH_MERGING"]

SCHEMA_PATH_CURRENT = Path(tempfile.gettempdir()).joinpath("tempdb.sql")

SCHEMA_PATH_DIFFING = Path(tempfile.gettempdir()).joinpath("diffdb.sql")

DB_CONFIG_DEFAULT = {"host": POSTGRES_DEFAULT_HOST,
                       "port": POSTGRES_DEFAULT_PORT,
                       "username": POSTGRES_DEFAULT_USERNAME,
                       "password": POSTGRES_DEFAULT_PASSWORD,
                       "dbname": POSTGRES_DEFAULT_USERNAME}

DB_CONFIG_MERGING = {**DB_CONFIG_DEFAULT, "port": 9900}

DB_CONFIG_CURRENT = {**DB_CONFIG_DEFAULT, "port": 9995}

IGNORE_SQL_STATEMENTS_HEAD = {
    'alter extension "pg_stat_statements" update to'
}

IGNORE_SQL_STATEMENTS_COMPLETE = {
    'alter table "hdb_catalog"."event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."event_log" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_action_log" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_cron_event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_cron_events" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_scheduled_event_invocation_logs" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_scheduled_events" alter column "id" set default public.gen_random_uuid();',
    'alter table "hdb_catalog"."hdb_version" alter column "hasura_uuid" set default public.gen_random_uuid();'
}

PERFORM_DATABASE_MIGRATION = os.environ.get("PERFORM_DATABASE_MIGRATION")
