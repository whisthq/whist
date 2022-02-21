#!/bin/bash

# Deploy the services project to Heroku.

# Arguments
# ${1}: REGION_IMAGE_MAP: map of instance image ids by region.
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

SCALING_SERVICE_DIR="./backend/services/scaling-service"
DEPLOY_DIR="$SCALING_SERVICE_DIR/deploy"
IMAGE_FILE="$DEPLOY_DIR/images.json"
PROCFILE="$DEPLOY_DIR/Procfile"


# Copy the binary to the scaling service deploy directory. This is necessary because we will use as standalone repo.
mkdir -p "$DEPLOY_DIR" && cp ./backend/services/build/scaling-service "$DEPLOY_DIR"

# Write region image map to var file so the Procfile can read it.
echo "$REGION_IMAGE_MAP" > "$IMAGE_FILE"

# Write Procfile
echo -e "scaling: ./scaling-service" > "$PROCFILE"

# populate the deploy/ directory
mv "$DEPLOY_DIR" ..
git switch --orphan deploy-branch
git clean -dfx # remove any .gitignored files that might remain
mv ../deploy/* .
git add .
git commit -m "scaling-service deploy for $MONOREPO_COMMIT_HASH"

# Create null buildpack, see: https://elements.heroku.com/buildpacks/ryandotsmith/null-buildpack
heroku buildpacks:set http://github.com/ryandotsmith/null-buildpack.git -a "$HEROKU_APP_NAME" || true

# Push deploy directory to Heroku
echo "Deploying scaling service..."
git push -f heroku-whist-scaling-service deploy-branch:master

# Scale Heroku dyno to start the scaling process
heroku ps:scale scaling=1 -a "$HEROKU_APP_NAME"
