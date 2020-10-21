#!/bin/bash

set -Eeuo pipefail

app_path=${1%/}
repo_name=fractal/$app_path
remote_tag=$2
region=${3:-us-east-1}
mount=${4:-}
ecr_uri=$(aws ecr get-authorization-token --region $region --query authorizationData[0].proxyEndpoint --output text | cut -c 9-)
image=$ecr_uri/$repo_name:$remote_tag

aws ecr get-login-password --region $region | docker login --username AWS --password-stdin $ecr_uri

docker pull $image

./run_container_image.sh $image $mount
