#!/bin/bash

# Deploy the services project to Heroku.

# Arguments
# ${1}: REGION_IMAGE_MAP: map of instance image ids by region.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Working directory is monorepo root
cd "$DIR/../../.."

REGION_IMAGE_MAP=${1}
SCALING_SERVICE_DIR="./backend/services/scaling-service"
DEPLOY_DIR="$SCALING_SERVICE_DIR/deploy"
IMAGE_FILE="$DEPLOY_DIR/images.json"
PROCFILE="$DEPLOY_DIR/Procfile"

# Copy the binary to the scaling service bin directory. This is necessary because we will use as standalone repo.
mkdir -p "$DEPLOY_DIR" && cp ./backend/services/build/scaling-service "$DEPLOY_DIR"

# Write region image map to var file so the Procfile can read it.
echo "$REGION_IMAGE_MAP" > "$IMAGE_FILE"

# Write Procfile
echo -e "web: ./scaling-service" > "$PROCFILE"

# Persist changes on local repo so that they are read by splitsh
git add .
git commit -m "Create deploy directory for scaling service"

# Use splitsh to checkout to standalone scaling service repo.
echo "Checking out the scaling service folder as a standalone git repo..."
splitsh-lite --prefix backend/services/scaling-service/deploy --target refs/heads/workflows-private/scaling-service
git checkout workflows-private/scaling-service

# Create null buildpack, see: https://elements.heroku.com/buildpacks/ryandotsmith/null-buildpack
heroku buildpacks:set http://github.com/ryandotsmith/null-buildpack.git -a "$HEROKU_APP_NAME"

# Push deploy directory to Heroku
echo "Deploying scaling service..."
git push -f heroku-whist-scaling-service workflows-private/scaling-service:master
