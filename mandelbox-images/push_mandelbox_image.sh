#!/bin/bash

# This script takes a mandelbox image that was built locally, tags it with the proper
# naming convention and Git hash, and uploads it to GitHub Container Registry (GHCR).
# Arguments:
#   $1 - app name (e.g. browsers/chrome)
#   $2 - deploy environment (i.e. prod OR staging OR dev)

set -Eeuo pipefail

# Parameters of the local image to push to GHCR
git_hash=$(git rev-parse HEAD)
local_name=fractal/$1
local_tag=current-build
deploy_env=${2:-nodeployenv}
deploy_name=fractal/$deploy_env/$1
ghcr_uri=ghcr.io

# Authenticate to GHCR
echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri

# Tag the image following the Fractal naming convention
docker tag $local_name:$local_tag $ghcr_uri/$deploy_name:$git_hash

# Upload to GHCR
docker push $ghcr_uri/$deploy_name:$git_hash
