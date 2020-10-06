#!/bin/bash

git_hash=$(git rev-parse --short HEAD)

# build container with protocol inside it
docker build -f $1/Dockerfile.20 $1 -t fractal/$1:$git_hash.20
