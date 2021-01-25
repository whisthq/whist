#!/bin/bash

# exit on error
set -o errexit

# we must be in the tests folder, so the folder `tests` cannot exist
if [ -d tests ]; then
    echo "Please cd into tests and then run this script."
    exit 1
fi

echo "=== Make sure to run tests/setup/setup_tests.sh once prior to this ==="

# if in CI, run setup tests and set env vars
if [ ! -z $IN_CI ]; then
    # these are needed to migrate schema/data
    export POSTGRES_REMOTE_URI=$DATABASE_URL # set in config vars on Heroku
    export POSTGRES_LOCAL_URI=$HEROKU_POSTGRES_URL # set in app.json
    cd setup
    bash setup_tests.sh
    cd ..
    # app can look at POSTGRES_URI to connect to db. point to local CI db
    export POSTGRES_URI=$POSTGRES_LOCAL_URI
else
    # add env vars to current env. these tell us the host, db, role, pwd
    export $(cat ../docker/.env | xargs)

    # override POSTGRES_HOST and POSTGRES_PORT to be local
    export POSTGRES_HOST="localhost"
    export POSTGRES_PORT="9999"
fi

# we need to cd back out of tests into root dir for main-webserver
cd ..
# pass args to pytest
pytest "$@"

