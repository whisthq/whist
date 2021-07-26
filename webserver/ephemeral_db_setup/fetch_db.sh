#!/bin/bash

# fetch db data for the following tables:
# 1. sales.email_templates
# 2. hardware.region_to_ami
# 3. hardware.supported_app_images
# Either POSTGRES_URI is provided, or (POSTGRES_HOST, POSTGRES_USER, POSTGRES_DB, POSTGRES_PASSWORD)

# exit on error and missing env var
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `webserver/db_setup`
cd "$DIR"

# first fetch the current dev db schema
if [ -f db_data.sql ]; then
    echo "Found existing data sql scripts. Skipping fetching db."
    exit 0
fi

POSTGRES_URI=${POSTGRES_URI:=""}

# retrieve db info depending on if a URI is given or host/db/user. we check the prefix for a URI
if [[ ^$POSTGRES_URI =~ "postgres://" ]]; then
    echo "===  Retrieving DB data  === \n"
    (pg_dump -d $POSTGRES_URI --data-only --column-inserts -t sales.email_templates -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql

else
    # pg_dump will look at this and skip asking for a prompt
    export PGPASSWORD=$POSTGRES_PASSWORD

    echo "=== Retrieving DB data ==="
    echo ""
    (pg_dump -h "$POSTGRES_HOST" -U "$POSTGRES_USER" -d "$POSTGRES_DB" --data-only --column-inserts -t sales.email_templates -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
fi
