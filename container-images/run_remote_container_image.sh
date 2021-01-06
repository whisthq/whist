#!/bin/bash

set -Eeuo pipefail

app_path=${1%/}
repo_name=fractal/$app_path
remote_tag=$2
mount=${3:-}
ghcr_uri=ghcr.io
image=$ghcr_uri/$repo_name:$remote_tag

echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri

docker pull $image

./run_container_image.sh $image $mount
