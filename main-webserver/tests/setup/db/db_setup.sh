#!/bin/bash

# in CI we just run the (modified) download script
if [ -z $IN_CI ]; then
  echo "=== Initializing db for CI ===\n"
  python create_ci_db_schema.py

  # copy (slightly modified; see create_ci_db_schema.py) schema
  psql -d $POSTGRES_LOCAL_HOST -f db_schema.sql
  echo "===   Errors are ok as long as the db was made   ===\n"

  # copy select data
  echo "===             Putting data into db             ===\n"
  psql -d $POSTGRES_LOCAL_HOST -f db_data.sql

  exit 0
fi

# otherwise, run the typical setup script
if [ -z $POSTGRES_LOCAL_HOST ] || [ -z $POSTGRES_USER ] || [ -z $POSTGRES_DB ]; then
  echo "Missing one of these env vars: POSTGRES_LOCAL_HOST, POSTGRES_USER, POSTGRES_DB"
  exit 1
fi

# retrieve db info depending on if a URI is given or host/db/user. we check the prefix for a URI
if [[ ^$POSTGRES_LOCAL_HOST =~ "postgres://" ]]; then
  # prefix is a URI, use URI method

  echo "=== Make sure to update the schema periodically ===\n"
  echo "===            Initializing db                  ===\n"

  # make user. initially just "postgres" user exists
  cmds="CREATE ROLE $POSTGRES_USER WITH LOGIN CREATEDB;\q"
  psql -U postgres -d $POSTGRES_LOCAL_HOST <<< $cmds

  cmds="CREATE DATABASE $POSTGRES_DB;\q"
  psql -U $POSTGRES_USER -d $POSTGRES_LOCAL_HOST <<< $cmds

  # copy schema
  psql -U $POSTGRES_USER -d $POSTGRES_LOCAL_HOST -f db_schema.sql
  echo "===   Errors are ok as long as the db was made   ===\n"

  # copy select data
  echo "===             Putting data into db             ===\n"
  psql -U $POSTGRES_USER -d $POSTGRES_LOCAL_HOST -f db_data.sql

else
  # need user/db env vars, but first check they exist

  # if this is not defined in env, use default of 9999
  if [-z $POSTGRES_LOCAL_PORT]; then
    POSTGRES_LOCAL_PORT="9999"
  fi

  echo "=== Make sure to update the schema periodically ===\n"
  echo "===            Initializing db                  ===\n"

  # make user. initially just "postgres" user exists
  cmds="CREATE ROLE $POSTGRES_USER WITH LOGIN CREATEDB;\q"
  psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U postgres -d postgres <<< $cmds

  cmds="CREATE DATABASE $POSTGRES_DB;\q"
  psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_USER -d postgres <<< $cmds

  # copy schema
  psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_USER -d $POSTGRES_DB -f db_schema.sql
  echo "===   Errors are ok as long as the db was made   ===\n"

  # copy select data
  echo "===             Putting data into db             ===\n"
  psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_USER -d $POSTGRES_DB -f db_data.sql

fi

