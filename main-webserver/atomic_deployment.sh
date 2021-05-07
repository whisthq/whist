#!/bin/bash

# Atomically apply database migrations to the current webserver
# this 
# Arguments
# ${1}: HEROKU_APP_NAME: name of webserver Heroku app. Ex: fractal-dev-server.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is monorepo root
cd "$DIR/.."

HEROKU_APP_NAME=${1}

# Get the DB associated with the app. If this fails, the entire deploy will fail.
DB_URL=$(heroku config:get DATABASE_URL --app "${HEROKU_APP_NAME}")

# Install db migration dependencies. If this fails, the entire deploy will fail.
echo "Installing db migration dependencies..."
pip install -r main-webserver/db_migration/requirements.txt

echo "Checking out the main-webserver folder as a standalone git repo..."
splitsh-lite --prefix main-webserver --target refs/heads/workflows-private/main-webserver
git checkout workflows-private/main-webserver
# we are now in the main-webserver folder as a standalone git repo

# tmpfolder=$(mktemp -d)
tmpfolder=/var/folders/0x/q00z8t8s3xqb369kpg540bjc0000gn/T/tmp.3zuQmBPt
echo "Created a temporary folder to store schema information at $tmpfolder"

CURRENT_DB_SCHEMA_PATH="$tmpfolder/current_live_schema.sql"
NEW_DB_SCHEMA_PATH="db_migration/schema.sql"
OUT_DIFF="$tmpfolder/diff.sql"

echo "Getting the current schema..."
pg_dump --no-owner --no-privileges --schema-only "${DB_URL}" >> $CURRENT_DB_SCHEMA_PATH

echo "Calling schema diff script..."
set +e # allow any exit-code; we will semantically parse this
python db_migration/schema_diff.py ${CURRENT_DB_SCHEMA_PATH} ${NEW_DB_SCHEMA_PATH} ${OUT_DIFF}
DIFF_EXIT_CODE=$?
set -e # undo allowing any exit-code

echo "Schema diff exit code: $DIFF_EXIT_CODE"

if [ $DIFF_EXIT_CODE == "2" ] || [ $DIFF_EXIT_CODE == "3" ]; then
    # a diff exists, now apply it atomically by first pausing the webserver

    # stop webserver. TODO: parse how many dynos exist currently and
    # restore that many as opposed to just restoring to 1 dyno
    heroku ps:scale web=0 --app ${HEROKU_APP_NAME}
    heroku ps:scale celery=0 --app ${HEROKU_APP_NAME}

    # apply diff safely, knowing nothing is happening on webserver
    psql --single-transaction --file $OUT_DIFF $DB_URL

    heroku ps:scale web=1 --app $HEROKU_APP_NAME
    heroku ps:scale celery=1 --app $HEROKU_APP_NAME
elif [ $DIFF_EXIT_CODE == "0" ]; then
    echo "No diff. Continuing redeploy."
else
    echo "Diff script exited poorly. We are not redeploying the webserver because"
    echo "the ORM might be inconsistent with the live webserver db. This is an"
    echo "extremely serious error and should be investigated immediately."
    exit 1
fi

echo "Redeploying webserver..."

ls

# this should redeploy the webserver with code that corresponds to the new schema
# git push -f heroku-fractal-server workflows-private/main-webserver:master

