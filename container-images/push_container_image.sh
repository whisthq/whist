#!/bin/bash

set -Eeuo pipefail

git_hash=$(git rev-parse HEAD)
local_name=fractal/$1
local_tag=current-build
ghcr_uri=ghcr.io

echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri

docker tag $local_name:$local_tag $ghcr_uri/$local_name/test-4k:$git_hash
docker push $ghcr_uri/$local_name/test-4k:$git_hash
