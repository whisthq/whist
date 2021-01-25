#!/bin/bash

# in CI we can just run the modified download script against the db, which already
# has an initialized user/database. CI uses URI connection method.
if [ ! -z $IN_CI ]; then
  echo "=== Initializing db for CI ===\n"

  # copy (slightly modified; see modify_ci_db_schema.py) schema
  psql -d $POSTGRES_LOCAL_URI -f db_schema.sql
  echo "===   Errors are ok as long as the db was made   ===\n"

  # copy select data
  echo "===             Putting data into db             ===\n"
  psql -d $POSTGRES_LOCAL_URI -f db_data.sql

  exit 0
fi

# otherwise, run the typical setup script against a fresh, empty db. we do not use
# the URI method locally
if [ -z $POSTGRES_LOCAL_HOST ]; then
  echo "Must provide POSTGRES_LOCAL_HOST"
  exit 1
fi

# these vars must exist to make local db look like remote. we don't need a password
# because local db trusts all incoming connections
if [ -z $POSTGRES_LOCAL_USER ] || [ -z $POSTGRES_LOCAL_DB ]; then
  echo "Missing one of these env vars: POSTGRES_LOCAL_USER, POSTGRES_LOCAL_DB"
  exit 1
fi

# if this is not defined in env, use default of 9999
if [ -z $POSTGRES_LOCAL_PORT ]; then
  POSTGRES_LOCAL_PORT="9999"
fi

echo "=== Make sure to update the schema periodically ===\n"
echo "===            Initializing db                  ===\n"

# make user. initially just "postgres" user exists
cmds="CREATE ROLE $POSTGRES_LOCAL_USER WITH LOGIN CREATEDB;\q"
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U postgres -d postgres <<< $cmds

cmds="CREATE DATABASE $POSTGRES_LOCAL_DB;\q"
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_LOCAL_USER -d postgres <<< $cmds

# copy schema
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_LOCAL_USER -d $POSTGRES_LOCAL_DB -f db_schema.sql
echo "===   Errors are ok as long as the db was made   ===\n"

# copy select data
echo "===             Putting data into db             ===\n"
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_LOCAL_USER -d $POSTGRES_LOCAL_DB -f db_data.sql

