#!/bin/bash
set -Eeuo pipefail

git_hash=$(git rev-parse --short HEAD)
local_name=fractal/$1
local_tag=current-build
region=${2:-us-east-1}
ecr_uri=$(aws ecr get-authorization-token --region $region --query authorizationData[0].proxyEndpoint --output text | cut -c 9-)

aws ecr get-login-password --region $region | docker login --username AWS --password-stdin $ecr_uri

docker tag $local_name:$local_tag $ecr_uri/$local_name:$git_hash
docker tag $local_name:$local_tag $ecr_uri/$local_name:latest-stable
docker push $ecr_uri/fractal/$1
