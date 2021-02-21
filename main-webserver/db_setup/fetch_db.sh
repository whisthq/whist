#!/bin/bash

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `main-webserver/db_setup`
cd "$DIR"

POSTGRES_REMOTE_URI=${POSTGRES_REMOTE_URI:=""}

# retrieve db info depending on if a URI is given or host/db/user. we check the prefix for a URI
if [[ ^$POSTGRES_REMOTE_URI =~ "postgres://" ]]; then
    # prefix is a URI, use URI method
    echo "=== Retrieving DB schema ==="
    echo ""
    (pg_dump -C -d $POSTGRES_REMOTE_URI -s) > db_schema.sql

    echo "===  Retrieving DB data  ==="
    echo ""
    (pg_dump -d $POSTGRES_REMOTE_URI --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql

else
    # pg_dump will look at this and skip asking for a prompt
    export PGPASSWORD=$POSTGRES_REMOTE_PASSWORD

    echo "=== Retrieving DB schema ==="
    echo ""
    (pg_dump -C -h "$POSTGRES_REMOTE_HOST" -U "$POSTGRES_REMOTE_USER" -d "$POSTGRES_REMOTE_DB" -s) > db_schema.sql

    echo "=== Retrieving DB data ==="
    echo ""
    (pg_dump -h "$POSTGRES_REMOTE_HOST" -U "$POSTGRES_REMOTE_USER" -d "$POSTGRES_REMOTE_DB" --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
fi

# in CI we need to slightly modify the download script because Heroku does not let us run
# a superuser and do whatever we want to the db
IN_CI=${CI:=false} # default: false
if [ $IN_CI == "true" ]; then
    echo "=== Cleaning the script for CI ==="
    echo ""
    python modify_ci_db_schema.py
fi
