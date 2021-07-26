#!/bin/bash

# retrieve_config.sh

# Retrieve Heroku configuration variables that must be set in the local
# development environment. Write a series of lines of the form KEY=VALUE to a
# .env file in the directory containing this script. Print the contents of the
# file to standard output.

# Exit on subcommand errors
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `webserver/docker`
cd "$DIR"

outfile="$DIR/.env"

function fetch {
    heroku config:get AUTH0_DOMAIN CONFIG_DB_URL --shell \
        --app=fractal-dev-server \
        | tr -d \'
    heroku config:get DATABASE_URL --app=fractal-dev-server \
        | python3 "$DIR"/pgparse.py
}

fetch | tee "$outfile" && echo "Wrote $outfile" >&2
