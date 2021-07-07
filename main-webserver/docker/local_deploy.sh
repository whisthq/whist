#!/bin/bash

# usage: ./local_deploy
# deploys our webserver stack locally using Docker containers
# optional args:
# --down (tear down local deployment)

# exit on error
set -Eeuo pipefail
BRANCH=$(git branch --show-current)
COMMIT=$(git rev-parse --short HEAD)
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

# Make sure .env file exists
if [ ! -f .env ]; then
    echo "Did not find docker/.env file. Make sure you have run docker/retrieve_config.sh!"
    exit 1
fi

# add env vars to current env. these tell us the host, db, role, pwd
export $(cat .env | xargs)

bash ../ephemeral_db_setup/fetch_db.sh

# eph db configurations
export POSTGRES_HOST="localhost"
export POSTGRES_PORT="9999"

# POSTGRES_USER and POSTGRES_DB will be created in the db a few steps down with ../ephemeral_db_setup/db_setup.sh
# since this is run in a docker container, the @postgres_db allows our web/celery containers
# to talk to the postgres_db container. Our docker-compose sets up this container networking.
export DATABASE_URL=postgres://${POSTGRES_USER}@postgres_db/${POSTGRES_DB}

# launch images with ephemeral db
APP_GIT_BRANCH=$BRANCH APP_GIT_COMMIT=$COMMIT docker-compose up -d --build

# let ephemeral db prepare. Check connections using psql.
echo "Trying to connect to local db..."
cmds="\q"
while ! (psql -h $POSTGRES_HOST -p $POSTGRES_PORT -U postgres -d postgres <<< $cmds) &> /dev/null
do
    echo "Connection failed. Retrying in 2 seconds..."
    sleep 2
done

# set up the ephemeral  db
bash ../ephemeral_db_setup/db_setup.sh

echo "Success! Teardown when you are done with: docker/local_deploy.sh --down"
