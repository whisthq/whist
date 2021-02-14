#!/bin/bash

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `main-webserver/tests`
cd "$DIR"

# if in CI, run setup tests and set env vars
IN_CI=${CI:=false} # default: false
if [ $IN_CI == "true" ]; then
    # these are needed to migrate schema/data
    export POSTGRES_REMOTE_URI=$DATABASE_URL # set in config vars on Heroku
    export POSTGRES_LOCAL_URI=$HEROKU_POSTGRES_URL # set in app.json
    bash setup/setup_tests.sh
    # app can look at POSTGRES_URI to connect to db. point to local CI db
    export POSTGRES_URI=$POSTGRES_LOCAL_URI
else
    echo "=== Make sure to run tests/setup/setup_tests.sh once prior to this ==="

    # add env vars to current env. these tell us the host, db, role, pwd
    export $(cat ../docker/.env | xargs)

    # override POSTGRES_HOST and POSTGRES_PORT to be local
    export POSTGRES_HOST="localhost"
    export POSTGRES_PORT="9999"
fi

# pass args to pytest
(cd .. && pytest "$@")
