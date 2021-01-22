#!/bin/bash

# exit on error
set -o errexit

# check if we are in the tests folder
if [ -d tests ]; then
    echo "Please cd into tests and then run this script."
    exit 1
fi

echo "=== Make sure to run tests/setup/setup_tests.sh once prior to this ==="

# if in CI, run setup tests and set env vars
if [ ! -z $IN_CI ]; then
    # these are needed to migrate schema/data
    export POSTGRES_HOST=$DATABASE_URL
    export POSTGRES_LOCAL_HOST=$HEROKU_POSTGRES_URL # set in app.json
    cd setup
    bash setup_tests.sh
    cd ..
    # app looks at POSTGRES_URI to connect to db. point to CI db
    export POSTGRES_URI=$POSTGRES_LOCAL_HOST
else
    # add env vars to current env
    export $(cat ../../docker/.env | xargs)

    # override POSTGRES_HOST and POSTGRES_PORT to be local
    export POSTGRES_HOST="localhost"
    export POSTGRES_PORT="9999"
fi

# we need to cd back out of tests into root dir for main-webserver
cd ..
pytest --no-mock-aws

