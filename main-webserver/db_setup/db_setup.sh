#!/bin/bash

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `main-webserver/db_setup`
cd "$DIR"

# in CI we can just run the modified download script against the db, which already
# has an initialized user/database. CI uses URI connection method.
IN_CI=${CI:=false} # default: false
if [ $IN_CI == "true" ]; then
  echo "=== Initializing db for CI ==="

  # copy (slightly modified; see modify_ci_db_schema.py) schema
  psql -d $POSTGRES_LOCAL_URI -f db_schema.sql
  echo "===   Errors are ok as long as the db was made   ==="

  # copy select data
  echo "===             Putting data into db             ==="
  psql -d $POSTGRES_LOCAL_URI -f db_data.sql

  exit 0
fi

POSTGRES_LOCAL_PORT=${POSTGRES_LOCAL_PORT:=9999} # default: 9999

echo "=== Make sure to update the schema periodically ==="
echo "===            Initializing db                  ==="

# make user. initially just "postgres" user exists
cmds="CREATE ROLE $POSTGRES_LOCAL_USER WITH LOGIN CREATEDB;\q"
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U postgres -d postgres <<< $cmds

cmds="CREATE DATABASE $POSTGRES_LOCAL_DB;\q"
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_LOCAL_USER -d postgres <<< $cmds

# copy schema
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_LOCAL_USER -d $POSTGRES_LOCAL_DB -f db_schema.sql
echo "===   Errors are ok as long as the db was made   ==="

# copy select data
echo "===             Putting data into db             ==="
psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U $POSTGRES_LOCAL_USER -d $POSTGRES_LOCAL_DB -f db_data.sql

