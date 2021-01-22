#!/bin/bash

if [ -z $POSTGRES_HOST ]; then
  echo "Missing env var POSTGRES_HOST"
  exit 1
fi

# retrieve db info depending on if a URI is given or host/db/user. we check the prefix for a URI
if [[ ^$POSTGRES_HOST =~ "postgres://" ]]; then
  # prefix is a URI, use URI method
  
  echo "=== Retrieving DB schema === \n"
  (pg_dump -C -d $POSTGRES_HOST -s) > db_schema.sql

  echo "===  Retrieving DB data  === \n"
  (pg_dump -d $POSTGRES_HOST --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
  
else
  # need user/db env vars, but first check they exist

  if [ -z $POSTGRES_USER ] || [ -z $POSTGRES_PASSWORD ] || [ -z $POSTGRES_DB ]; then
    echo "Missing one of these env vars: POSTGRES_USER, POSTGRES_PASSWORD, POSTGRES_DB"
    exit 1
  fi

  # pg_dump will look at this and skip asking for a prompt
  export PGPASSWORD=$POSTGRES_PASSWORD

  echo "=== Retrieving DB schema ===\n"
  (pg_dump -C -h $POSTGRES_HOST -U $POSTGRES_USER -d $POSTGRES_DB -s) > db_schema.sql

  echo "=== Retrieving DB schema ===\n"
  (pg_dump -h $POSTGRES_HOST -U $POSTGRES_USER -d $POSTGRES_DB --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
fi

# in CI we need to slightly modify the download script because Heroku does not let us run
# a superuser and do whatever we want to the db
if [ ! -z $IN_CI ]; then
  echo "=== Cleaning the script for CI ===\n"
  python create_ci_db_schema.py
fi
