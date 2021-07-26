#!/bin/bash

# set up a db to look like the real ones.
# If the db is already initialized with a user (this happens in CI and review apps)
# we just apply the schema and data to $POSTGRES_URI. This must be explicitly
# indicated by setting DB_EXISTS=true in the environment. Default is false.
# Otherwise, we create a user and db before applying the schema and data.
# The environment variables (POSTGRES_HOST, POSTGRES_PORT, POSTGRES_USER, POSTGRES_DB)
# must be provided.

# exit on error and missing env var
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `webserver/db_setup`
cd "$DIR"

# check if the user is already initialized. this happens in CI and during review apps.
# if so, POSTGRES_URI should be provided
DB_EXISTS=${DB_EXISTS:=false} # default: false
if [ $DB_EXISTS == true ]; then
    # copy schema
    psql -d $POSTGRES_URI -f ../db_migration/schema.sql

    # copy specifically chosen data
    echo "===             Putting data into db             ==="
    psql -d "$POSTGRES_URI" -f db_data.sql

    exit 0
fi

# here, we just have a fresh postgres instance with a user called postgres
# we need to create the user/db before running the schema/data setup
# POSTGRES_HOST, POSTGRES_USER, POSTGRES_DB, and POSTGRES_PORT should be provided.

echo "===  Make sure to update the data periodically  ==="
echo "===            Initializing db                  ==="

# make user. initially just "postgres" user exists
cmds="CREATE ROLE $POSTGRES_USER WITH LOGIN CREATEDB;\q"
psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U postgres -d postgres <<< $cmds

# superuser privileges are needed to install pg_extensions needed for Hasura.
cmds="ALTER ROLE $POSTGRES_USER SUPERUSER;\q"
psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U postgres -d postgres <<< $cmds

cmds="CREATE DATABASE $POSTGRES_DB;\q"
psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -d postgres <<< $cmds

# copy schema
psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -d "$POSTGRES_DB" -f ../db_migration/schema.sql

# copy specifically chosen data
echo "===             Putting data into db             ==="
psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -d "$POSTGRES_DB" -f db_data.sql
