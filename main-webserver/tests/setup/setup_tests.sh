#!/bin/bash

# exit on error
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# make sure current directory is `main-webserver/tests/setup`
cd "$DIR"

# check if in CI; if so just run fetch and setup scripts then exit
IN_CI=${CI:=false} # default: false
if [ $IN_CI == "true" ]; then
    bash ../../db_setup/fetch_db.sh
    exit 0
fi

# Make sure .env file exists
if [ ! -f ../../docker/.env ]; then
    echo "Make sure you have run retrieve_config.sh"
    exit 1
fi

# add env vars to current env. these tell us the host, db, role, pwd
export $(cat ../../docker/.env | xargs)

# rename for clarity, as we have a remote host and a local host
export POSTGRES_REMOTE_HOST=$POSTGRES_HOST
export POSTGRES_REMOTE_USER=$POSTGRES_USER
export POSTGRES_REMOTE_PASSWORD=$POSTGRES_PASSWORD
export POSTGRES_REMOTE_DB=$POSTGRES_DB


if [ -f ../../db_setup/db_schema.sql ]; then
    echo "Found existing schema and data sql scripts. Skipping fetching db."
else
    bash ../../db_setup/fetch_db.sh
fi

docker-compose up -d --build

# let db prepare. TODO: make more robust
sleep 2

# local testing uses localhost db
export POSTGRES_LOCAL_HOST="localhost"
export POSTGRES_LOCAL_PORT="9999"
# we don't need a pwd because local db trusts all incoming connections
export POSTGRES_LOCAL_USER=$POSTGRES_REMOTE_USER
export POSTGRES_LOCAL_DB=$POSTGRES_REMOTE_DB
bash ../../db_setup/fetch_db.sh

echo "Success! Teardown when you are done with: docker-compose down"

