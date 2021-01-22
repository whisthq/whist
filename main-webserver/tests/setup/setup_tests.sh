#!/bin/bash

# exit on error
set -o errexit

# check if in CI; if so just run fetch and setup scripts then exit
# all env vars should be provided by caller
if [ ! -z $IN_CI ]; then
    cd db
    bash fetch_db.sh
    bash db_setup.sh
    cd ..
    exit 0
fi

if [ ! -f ../../docker/.env ]; then
    echo "Make sure you have run retrieve_config.sh"
    exit 1
fi

# add env vars to current env
export $(cat ../../docker/.env | xargs)

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
cd db
bash db_setup.sh
cd ..

echo "Success! Teardown when you are done with: docker-compose down"

