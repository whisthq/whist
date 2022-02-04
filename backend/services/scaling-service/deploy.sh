#!/bin/bash

# Deploy the scaling service to Heroku.

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
BUILD_DIR="$SCALING_SERVICE_DIR"
IMAGE_FILE="$SCALING_SERVICE_DIR/images.json"


# Copy the binary to the scaling service dir. This is necessary because we will use as standalone repo.
cp ./backend/services/build/scaling-service "$BUILD_DIR"

# Write region image map to var file so the Procfile can read it.
echo "$REGION_IMAGE_MAP" > "$IMAGE_FILE"

# Persist changes on local repo so that they are read by splitsh
git config --global user.name "whist Developers"
git config --global user.email "developers@whist.com"
git add .
git commit -m "Add scaling service build and images.json file for splitsh"

# Use splitsh to checkout to standalone scaling service repo.
echo "Checking out the scaling service folder as a standalone git repo..."
splitsh-lite --prefix backend/services/scaling-service --target refs/heads/workflows-private/scaling-service
git checkout workflows-private/scaling-service

# we are now in the scaling-service folder as a standalone git repo, push to Heroku.
echo "Redeploying scaling service..."
git push -f heroku workflows-private/scaling-service:master
