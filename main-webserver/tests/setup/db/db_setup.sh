#!/bin/bash
if [ -z "${POSTGRES_HOST}" ] || [ -z "${POSTGRES_USER}" ] || [ -z "${POSTGRES_DB}" ]; then
  echo "Missing one of these env vars: POSTGRES_HOST, POSTGRES_USER, POSTGRES_DB"
  exit 1
fi

# if this is not defined in env, use defaults
if [-z "${POSTGRES_LOCAL_HOST}"]; then
  POSTGRES_LOCAL_HOST="localhost"
fi

# if this is not defined in env, use defaults
if [-z "${POSTGRES_LOCAL_PORT}"]; then
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

echo "===             Putting data into db             ===\n"
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_USER -d $POSTGRES_DB -f db_data.sql
