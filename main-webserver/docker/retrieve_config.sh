#!/bin/bash

# retrieve_config.sh

# Retrieve Heroku configuration variables that must be set in the local
# development environment. Write a series of lines of the form KEY=VALUE to a
# .env file in the directory containing this script. Print the contents of the
# file to standard output.

set -eo pipefail

relpath=$(dirname "$0")
outfile="$relpath"/.env

function fetch {
    # As long as developers are going to be connecting to the database attached
    # to the development web application Heroku deployment, local deployments
    # should use the same secret key as that development deployment.
    heroku config:get CONFIG_DB_URL SECRET_KEY AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY --shell \
	   --app=fractal-dev-server
    heroku config:get DATABASE_URL --app=fractal-dev-server | \
	py "$relpath"/pgparse.py

    echo "HOT_RELOAD="
}

fetch | tee "$outfile" && echo "Wrote $outfile" >&2
