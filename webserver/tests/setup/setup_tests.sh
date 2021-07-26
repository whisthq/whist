#!/bin/bash

# exit on error
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# make sure current directory is `webserver/tests/setup`
cd "$DIR"

# Allow passing `--down` to spin down the docker-compose stack, instead of
# having to cd into this directory and manually run the command.
if [[ $* =~ [:space:]*--down[:space:]* ]]; then
    echo "Running \"docker-compose down\"..."
    docker-compose down
    exit 0
fi

# check if db has already been created. this happens in CI and review app contexts.
# in this case, we simply need to apply the schema and data to the ephemeral db.
DB_EXISTS=${DB_EXISTS:=false} # default: false
if [ $DB_EXISTS == true ]; then
    # use source db (dev, staging, prod) db to fetch data
    export POSTGRES_URI=$POSTGRES_SOURCE_URI
    bash ../../ephemeral_db_setup/fetch_db.sh

    # setup ephemeral db
    export POSTGRES_URI=$POSTGRES_DEST_URI
    # db itself was already created by Heroku; we just need to apply schema and insert data
    export DB_EXISTS=true
    bash ../../ephemeral_db_setup/db_setup.sh

    exit 0
fi

# we are running locally. We have a bit of extra work to do, namely:
# 1. load environment variables telling us about the location of the dev db
# 2. fetching db data using env variables
# 3. spinning up test mandelboxes
# 4. applying schema and inserting data into docker db

# Make sure .env file exists
if [ ! -f ../../docker/.env ]; then
    echo "Did not find docker/.env file. Make sure you have already run docker/retrieve_config.sh!"
    exit 1
fi

# load env vars, namely (POSTGRES_HOST, POSTGRES_USER, POSTGRES_PASSWORD, POSTGRES_DB)
export $(cat ../../docker/.env | xargs)

bash ../../ephemeral_db_setup/fetch_db.sh
BRANCH=$(git branch --show-current)
COMMIT=$(git rev-parse --short HEAD)

GIT_APP_BRANCH=$BRANCH GIT_APP_COMMIT=$COMMIT docker-compose up -d --build

# local testing uses localhost db. override POSTGRES_HOST and set POSTGRES_PORT.
export POSTGRES_HOST="localhost"
export POSTGRES_PORT="9999"

# let db prepare. Check connections using psql.
echo "Trying to connect to local db..."
cmds="\q"
while ! (psql -h $POSTGRES_HOST -p $POSTGRES_PORT -U postgres -d postgres <<< $cmds) &> /dev/null
do
    echo "Connection failed. Retrying in 2 seconds..."
    sleep 2
done

bash ../../ephemeral_db_setup/db_setup.sh
echo "Success! Teardown when you are done with: tests/setup/setup_tests.sh --down"
