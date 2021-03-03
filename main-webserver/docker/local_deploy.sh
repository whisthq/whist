#!/bin/bash

# usage: ./local_deploy
# deploys our webserver stack locally using Docker containers 
# optional args:
# --down (tear down local deployment)
# --use-dev-db (use the dev db instead of a local ephemeral db. See README about using this flag.)

# exit on error
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# make sure current directory is `main-webserver/docker`
cd "$DIR"

# Allow passing `--down` to spin down the docker-compose stack, instead of
# having to cd into this directory and manually run the command.
if [[ $* =~ [:space:]*--down[:space:]* ]]; then
  echo "Running \"docker-compose down\". Ignore any warnings about unset variables."
  docker-compose down
  exit 0
fi

USE_DEV_DB=false
# Allow passing `--use-dev-db` to use the dev database
if [[ $* =~ [:space:]*--use-dev-db[:space:]* ]]; then
  echo "WARNING: Using the dev db."
  USE_DEV_DB=true
fi

# Make sure .env file exists
if [ ! -f .env ]; then
    echo "Did not find docker/.env file. Make sure you have run docker/retrieve_config.sh!"
    exit 1
fi

# add env vars to current env. these tell us the host, db, role, pwd
export $(cat .env | xargs)

# rename for clarity, as we have a remote host and a local host
export POSTGRES_REMOTE_HOST=$POSTGRES_HOST
export POSTGRES_REMOTE_USER=$POSTGRES_USER
export POSTGRES_REMOTE_PASSWORD=$POSTGRES_PASSWORD
export POSTGRES_REMOTE_DB=$POSTGRES_DB

if [ $USE_DEV_DB == "true" ]; then
    export DATABASE_URL=postgres://${POSTGRES_REMOTE_USER}:${POSTGRES_REMOTE_PASSWORD}@${POSTGRES_REMOTE_HOST}/${POSTGRES_REMOTE_DB}

    # launch all images but dev db
    docker-compose up --build -d redis web celery # don't spin up postgres_db

else
    # first fetch the current dev db schema
    if [ -f ../db_setup/db_schema.sql ]; then
        echo "Found existing schema and data sql scripts. Skipping fetching db."
    else
        bash ../db_setup/fetch_db.sh
    fi

    # eph db configurations
    export POSTGRES_LOCAL_HOST="localhost"
    export POSTGRES_LOCAL_PORT="9999"
    # we don't need a pwd because local db trusts all incoming connections
    export POSTGRES_LOCAL_USER=$POSTGRES_REMOTE_USER
    export POSTGRES_LOCAL_DB=$POSTGRES_REMOTE_DB

    export DATABASE_URL=postgres://${POSTGRES_LOCAL_USER}@postgres_db/${POSTGRES_LOCAL_DB}

    # launch images with ephemeral db
    docker-compose up -d --build

    # let ephemeral db prepare. Check connections using psql.
    echo "Trying to connect to local db..."
    cmds="\q"
    while ! (psql -h $POSTGRES_LOCAL_HOST -p $POSTGRES_LOCAL_PORT -U postgres -d postgres <<< $cmds) &> /dev/null
    do
        echo "Connection failed. Retrying in 2 seconds..."
        sleep 2
    done
    
    # set up the ephemeral  db
    bash ../db_setup/db_setup.sh
fi

echo "Success! Teardown when you are done with: docker/local_deploy.sh --down"
