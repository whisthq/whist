#!/bin/bash
set -Eeuo pipefail

git_hash=$(git rev-parse HEAD)
local_name=fractal/$1
local_tag=current-build
region=${2:-us-east-1}
ecr_uri=$(aws ecr get-authorization-token --region $region --query authorizationData[0].proxyEndpoint --output text | cut -c 9-)

aws ecr get-login-password --region $region | docker login --username AWS --password-stdin $ecr_uri

# create ecr repository if it doesn't already exist
aws ecr describe-repositories --region $region --repository-names $local_name > /dev/null 2> /dev/null || { echo "Repository $local_name does not exist in region $region, creating..." ; aws ecr create-repository --region $region --repository-name $local_name > /dev/null ; }

docker tag $local_name:$local_tag $ecr_uri/$local_name:$git_hash
docker tag $local_name:$local_tag $ecr_uri/$local_name:latest-stable
docker push $ecr_uri/$local_name
