#!/bin/bash

# exit on error
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# make sure current directory is `main-webserver/tests/setup`
cd "$DIR"

# Allow passing `--down` to spin down the docker-compose stack, instead of
# having to cd into this directory and manually run the command.
if [[ $* =~ [:space:]*--down[:space:]* ]]; then
  echo "Running \"docker-compose down\"..."
  docker-compose down
  exit 0
fi

# check if in CI; if so just run fetch and setup scripts then exit
IN_CI=${CI:=false} # default: false
if [ $IN_CI == "true" ]; then
    bash ../../db_setup/fetch_db.sh
    bash ../../db_setup/db_setup.sh
    exit 0
fi

# Make sure .env file exists
if [ ! -f ../../docker/.env ]; then
    echo "Did not find docker/.env file. Make sure you have already run docker/retrieve_config.sh!"
    exit 1
fi

# Make sure we have dummy SSL certificates. Note that we don't care if we
# overwrite existing ones.
bash ../../dummy_certs/create_dummy_certs.sh

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

# local testing uses localhost db
export POSTGRES_LOCAL_HOST="localhost"
export POSTGRES_LOCAL_PORT="9999"
# we don't need a pwd because local db trusts all incoming connections
export POSTGRES_LOCAL_USER=$POSTGRES_REMOTE_USER
export POSTGRES_LOCAL_DB=$POSTGRES_REMOTE_DB
# let db prepare. Check connections using psql.
echo "Trying to connect to local db..."
cmds="\q"
while ! (psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U postgres -d postgres <<< $cmds) &> /dev/null
do
    echo "Connection failed. Retrying in 2 seconds..."
    sleep 2
done

bash ../../db_setup/db_setup.sh
echo "Success! Teardown when you are done with: tests/setup/setup_tests.sh --down"
