#!/bin/bash

# exit on error and missing env var
set -Eeuo pipefail

POSTGRES_REMOTE_URI=${POSTGRES_REMOTE_URI:=""}

# retrieve db info depending on if a URI is given or host/db/user. we check the prefix for a URI
if [[ ^$POSTGRES_REMOTE_URI =~ "postgres://" ]]; then
  # prefix is a URI, use URI method
  echo "=== Retrieving DB schema === \n"
  (pg_dump -C -d $POSTGRES_REMOTE_URI -s) > db_schema.sql

  echo "===  Retrieving DB data  === \n"
  (pg_dump -d $POSTGRES_REMOTE_URI --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
  
else
  # pg_dump will look at this and skip asking for a prompt
  export PGPASSWORD=$POSTGRES_REMOTE_PASSWORD

  echo "=== Retrieving DB schema ===\n"
  (pg_dump -C -h $POSTGRES_REMOTE_HOST -U $POSTGRES_REMOTE_USER -d $POSTGRES_REMOTE_DB -s) > db_schema.sql

  echo "=== Retrieving DB data ===\n"
  (pg_dump -h $POSTGRES_REMOTE_HOST -U $POSTGRES_REMOTE_USER -d $POSTGRES_REMOTE_DB --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
fi

# in CI we need to slightly modify the download script because Heroku does not let us run
# a superuser and do whatever we want to the db
IN_CI=${IN_CI:=false} # default: false
if [ $IN_CI == "true" ]; then
  echo "=== Cleaning the script for CI ===\n"
  python modify_ci_db_schema.py
fi
