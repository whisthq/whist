#!/bin/bash

git_hash=$(git rev-parse --short HEAD)
local_name=fractal/$1
local_tag=$git_hash.$2
region=${3:-us-east-1}
ecr_uri=$(aws ecr get-authorization-token --region $region --query authorizationData[0].proxyEndpoint --output text | cut -c 9-)

aws ecr get-login-password --region $region | docker login --username AWS --password-stdin $ecr_uri

docker tag $local_name:$local_tag $ecr_uri/$local_name:$local_tag
docker tag $local_name:$local_tag $ecr_uri/$local_name:latest-stable
docker push $ecr_uri/fractal/$1
