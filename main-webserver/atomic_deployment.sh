#!/bin/bash

# Atomically apply database migrations to the current webserver
# This is specifically written to run in the main-webserver deployment workflow context.
# It needs the following to be setup:
# .github/workflows/helpers/notifications is installed in the PYTHONPATH
# splitsh-lite (https://github.com/splitsh/lite)
# Heroku CLI

# Arguments
# ${1}: HEROKU_APP_NAME: name of webserver Heroku app. Ex: fractal-dev-server.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is monorepo root
cd "$DIR/.."

HEROKU_APP_NAME=${1}
MIGRA_EXIT_CODE=${2}
SQL_DIFF_STRING=${3}

# if true, a future step will send a slack notification
echo "DB_MIGRATION_PERFORMED=false" >> "${GITHUB_ENV}"

# Get the DB associated with the app. If this fails, the entire deploy will fail.
DB_URL=$(heroku config:get DATABASE_URL --app "${HEROKU_APP_NAME}")
echo "APP: $HEROKU_APP_NAME, DB URL: $DB_URL"

echo "Checking out the main-webserver folder as a standalone git repo..."
splitsh-lite --prefix main-webserver --target refs/heads/workflows-private/main-webserver
git checkout workflows-private/main-webserver
# we are now in the main-webserver folder as a standalone git repo

if [ $MIGRA_EXIT_CODE == "2" ] || [ $MIGRA_EXIT_CODE == "3" ]; then
    # a diff exists, now apply it atomically by first pausing the webserver

    echo "Migra SQL diff:"
    echo "${SQL_DIFF_STRING}"

    # stop webserver. TODO: parse how many dynos exist currently and
    # restore that many as opposed to just restoring to 1 dyno
    heroku ps:scale web=0 --app "${HEROKU_APP_NAME}"

    # apply diff safely, knowing nothing is happening on webserver
    # by passing "-" to --file, we can stream sql commands through stdin
    echo "${SQL_DIFF_STRING}" | psql --single-transaction --file "-" "${DB_URL}"

    echo "Redeploying webserver..."
    # this should redeploy the webserver with code that corresponds to the new schema
    git push -f heroku-fractal-server workflows-private/main-webserver:master

    # bring webserver back online
    heroku ps:scale web=1 --app "${HEROKU_APP_NAME}"

    echo "DB_MIGRATION_PERFORMED=true" >> "${GITHUB_ENV}"

elif [ $MIGRA_EXIT_CODE == "0" ]; then
    echo "No diff. Continuing redeploy."

    echo "Redeploying webserver..."
    # this should redeploy the webserver with code that corresponds to the new schema
    git push -f heroku-fractal-server workflows-private/main-webserver:master

else
    echo "Diff script exited poorly. We are not redeploying the webserver because"
    echo "the ORM might be inconsistent with the live webserver db. This is an"
    echo "extremely serious error and should be investigated immediately."
    exit 1
fi

