#!/bin/bash

# Deploy the scaling-service project to Heroku.

# Arguments
# ${1}: REGION_IMAGE_MAP: map of instance image IDs by region.
# ${2}: HEROKU_APP_NAME: name of the Heroku app we are deploying to.
# ${3}: MONOREPO_COMMIT_HASH: commit SHA that triggered the deploy.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is monorepo root
cd "$DIR/../../.."

REGION_IMAGE_MAP=${1}
HEROKU_APP_NAME=${2}
MONOREPO_COMMIT_HASH=${3}
MIGRA_EXIT_CODE=${4}
SQL_DIFF_STRING=${5}

SCALING_SERVICE_DIR="./backend/services/scaling-service"
DEPLOY_DIR="$SCALING_SERVICE_DIR/deploy"
IMAGE_FILE="$DEPLOY_DIR/images.json"
PROCFILE="$DEPLOY_DIR/Procfile"

####################################################
# Deploy Scaling Service
####################################################

# `deploy_scaling_service` will perform the necessary setup for
# deploying a new version of the scaling service to Heroku and
# push the changes.
# Args: none
deploy_scaling_service() {
  # Copy the binary to the scaling-service deploy directory. This is necessary because we will use it as standalone repo.
  mkdir -p "$DEPLOY_DIR" && cp ./backend/services/build/scaling-service "$DEPLOY_DIR"

  # Write region image map to var file so the Procfile can read it.
  echo "$REGION_IMAGE_MAP" > "$IMAGE_FILE"

  # Write Procfile
  echo -e "web: ./scaling-service" > "$PROCFILE"

  # Populate the deploy/ directory
  mv "$DEPLOY_DIR" ..
  git switch --orphan deploy-branch
  git clean -dfx # remove any .gitignored files that might remain
  mv ../deploy/* .
  git add .
  git commit -m "scaling-service deploy for $MONOREPO_COMMIT_HASH"

  # Create null buildpack, see: https://elements.heroku.com/buildpacks/ryandotsmith/null-buildpack, this is
  # necessary since Heroku requires a buildpack for every app
  heroku buildpacks:set http://github.com/ryandotsmith/null-buildpack.git -a "$HEROKU_APP_NAME" &> /dev/null || true

  # Push deploy directory to Heroku. Heroku is very bad, and will often fail to accept the deploy with:
  # error: RPC failed; HTTP 504 curl 22 The requested URL returned error: 504
  # To get around this, we simply try to push the deploy until it succeeds.
  echo "Deploying scaling-service..."
  count=0
  until $(git push -f heroku-whist-scaling-service deploy-branch:master) || ((count++ >= 5))
  do
    sleep 1
    echo "Failed to deploy to Heroku on attempt #$count of 5, retrying..."
  done

  # Scale Heroku dyno to start the web process
  heroku ps:scale web=1 -a "$HEROKU_APP_NAME"
}

# if true, a future step will send a slack notification
echo "DB_MIGRATION_PERFORMED=false" >> "${GITHUB_ENV}"

# Get the DB associated with the app. If this fails, the entire deploy will fail.
DB_URL=$(heroku config:get DATABASE_URL --app "${HEROKU_APP_NAME}")
echo "APP: $HEROKU_APP_NAME, DB URL: $DB_URL"

if [ "$MIGRA_EXIT_CODE" == "2" ] || [ "$MIGRA_EXIT_CODE" == "3" ]; then
  # a diff exists, now apply it atomically by first pausing the scaling service
  echo "Migra SQL diff:"
  echo "${SQL_DIFF_STRING}"

  # stop scaling service
  heroku ps:scale web=0 --app "${HEROKU_APP_NAME}"

  # Apply diff safely, knowing nothing is happening on scaling service.  Note that
  # we don't put quotes around SQL_DIFF_STRING to prevent. `$function$` from
  # turning into `$`.
  echo "${SQL_DIFF_STRING}" | psql -v ON_ERROR_STOP=1 --single-transaction "${DB_URL}"

  deploy_scaling_service

  echo "DB_MIGRATION_PERFORMED=true" >> "${GITHUB_ENV}"

elif [ "$MIGRA_EXIT_CODE" == "0" ]; then
  echo "No diff. Continuing redeploy."

  # this should redeploy the scaling service with code that corresponds to the new schema
  deploy_scaling_service

else
  echo "Diff script exited poorly. We are not redeploying the scaling service because"
  echo "the ORM might be inconsistent with the live scaling service db. This is an"
  echo "extremely serious error and should be investigated immediately."
  exit 1
fi

