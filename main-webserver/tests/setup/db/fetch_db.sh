#!/bin/bash

if [ -z $POSTGRES_HOST ] || [ -z $POSTGRES_PASSWORD ]; then
  echo "Missing one of these env vars: POSTGRES_HOST, POSTGRES_PASSWORD"
  exit 1
fi

# pg_dump will look at this and skip asking for a prompt
export PGPASSWORD=$POSTGRES_PASSWORD

# retrieve db info depending on if a URI is given or host/db/user. we check the prefix for a URI
if [[ ^$POSTGRES_HOST =~ "postgres://" ]]; then
  # prefix is a URI, use URI method
  
  echo "=== Retrieving DB schema === \n"
  (pg_dump -C -d $POSTGRES_HOST -s) > db_schema.sql

  echo "===  Retrieving DB data  === \n"
  (pg_dump -d $POSTGRES_HOST --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
  
else
  # need user/db env vars, but first check they exist

  if [ -z $POSTGRES_USER ] || [ -z $POSTGRES_DB ]; then
    echo "Missing one of these env vars: POSTGRES_USER, POSTGRES_DB"
    exit 1
  fi

  echo "=== Retrieving DB schema === \n"
  (pg_dump -C -h $POSTGRES_HOST -U $POSTGRES_USER -d $POSTGRES_DB -s) > db_schema.sql

  echo "=== Retrieving DB schema === \n"
  (pg_dump -h $POSTGRES_HOST -U $POSTGRES_USER -d $POSTGRES_DB --data-only --column-inserts -t hardware.region_to_ami -t hardware.supported_app_images) > db_data.sql
fi