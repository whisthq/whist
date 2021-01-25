#!/bin/bash

# exit on error
set -o errexit

# check if in CI; if so just run fetch and setup scripts then exit
if [ ! -z $IN_CI ]; then
    cd db
    bash fetch_db.sh
    bash db_setup.sh
    cd ..
    exit 0
fi

# make sure current directory is tests/setup
if [ ! -f ../../docker/.env ]; then
    echo "Make sure you have run retrieve_config.sh"
    exit 1
fi

# add env vars to current env. these tell us the host, db, role, pwd
export $(cat ../../docker/.env | xargs)
# all these env vars should exist in the docker/.env file.
if [ -z $POSTGRES_HOST ] || [ -z $POSTGRES_USER ] || [ -z $POSTGRES_PASSWORD ] || [ -z $POSTGRES_DB ]; then
    echo "Missing one of POSTGRES_HOST, POSTGRES_USER, POSTGRES_PASSWORD, POSTGRES_DB"
    exit 1
fi
# rename for clarity, as we have a remote host and a local host
export POSTGRES_REMOTE_HOST=$POSTGRES_HOST
export POSTGRES_REMOTE_USER=$POSTGRES_USER
export POSTGRES_REMOTE_PASSWORD=$POSTGRES_PASSWORD
export POSTGRES_REMOTE_DB=$POSTGRES_DB


if [ -f db/db_schema.sql ]; then
    echo "Found existing schema and data sql scripts. Skipping fetching db."
else
    cd db
    bash fetch_db.sh
    cd ..
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
cd db
bash db_setup.sh
cd ..

echo "Success! Teardown when you are done with: docker-compose down"

