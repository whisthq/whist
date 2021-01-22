#!/bin/bash

# exit on error
set -o errexit

echo "=== Make sure to run tests/setup/setup_tests.sh once prior to this ==="

# if in CI, run setup tests and set env vars
if [ ! -z $IN_CI ]; then
    # these are needed to migrate schema/data
    export POSTGRES_HOST=$DATABASE_URL
    export POSTGRES_LOCAL_HOST=$HEROKU_POSTGRES_URL # set in app.json
    cd setup
    bash setup_tests.sh
    cd ..
else
    # if local, setup_tests should already be run (once).
    # db is running at localhost:9999 so we set this
    export POSTGRES_LOCAL_HOST="localhost"
    export POSTGRES_PORT="9999"
fi

# app looks at POSTGRES_HOST to connect to db. override to point to testing db (CI or local)
export POSTGRES_HOST=$POSTGRES_LOCAL_HOST
pytest --no-mock-aws

