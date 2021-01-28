#!/bin/bash

# This script takes a container image that was built locally, tags it with the proper
# naming convetion and Git hash, and uploads it to GitHub Container Resgitry (GHCR).

set -Eeuo pipefail

# Parameters of the local container image to push to GHCR
git_hash=$(git rev-parse HEAD)
local_name=fractal/$1
local_tag=current-build
ghcr_uri=ghcr.io

# Authenticate to GHCR
echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri

# Tag the container image following the Fractal naming convention
docker tag $local_name:$local_tag $ghcr_uri/$local_name:$git_hash

# Upload to GHCR
docker push $ghcr_uri/$local_name:$git_hash
