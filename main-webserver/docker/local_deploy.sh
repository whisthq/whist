#!/bin/bash

# exit on error
set -Eeuo pipefail

CURRENT_DIR=`pwd`

# make sure current directory is docker
if [ ! -f .env ]; then
    echo "Make sure you have run retrieve_config.sh and are in the docker folder"
    exit 1
fi

# add env vars to current env. these tell us the host, db, role, pwd
export $(cat .env | xargs)

# rename for clarity, as we have a remote host and a local host
export POSTGRES_REMOTE_HOST=$POSTGRES_HOST
export POSTGRES_REMOTE_USER=$POSTGRES_USER
export POSTGRES_REMOTE_PASSWORD=$POSTGRES_PASSWORD
export POSTGRES_REMOTE_DB=$POSTGRES_DB


if [ -f ../db_setup/db_schema.sql ]; then
    echo "Found existing schema and data sql scripts. Skipping fetching db."
else
    # navigate to test directory to get the db
    cd ../db_setup
    bash fetch_db.sh
    cd $CURRENT_DIR
fi

# local deploy uses localhost db
export POSTGRES_LOCAL_HOST="localhost"
export POSTGRES_LOCAL_PORT="9999"
# we don't need a pwd because local db trusts all incoming connections
export POSTGRES_LOCAL_USER=$POSTGRES_REMOTE_USER
export POSTGRES_LOCAL_DB=$POSTGRES_REMOTE_DB
# launch images using above LOCAL env vars
docker-compose up -d --build

# let db prepare. TODO: make more robust
sleep 2

cd ../db_setup
bash db_setup.sh
cd $CURRENT_DIR

echo "Success! Teardown when you are done with: docker-compose down"


