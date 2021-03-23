#!/bin/bash

# This script builds the Fractal protocol server for Linux Ubuntu 20.04
# This script calls docker-create-builder.sh and docker-run-builder.sh, which build and run the Docker container
# defined by Dockerfile

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Set Cmake release flag
if [[ ${1:-''} == release ]]; then
    release_tag='-D CMAKE_BUILD_TYPE=Release'
else
    release_tag=''
fi

# Build protocol
# Note: we clean build to prevent Cmake caching issues
docker build . \
    --build-arg uid=$(id -u ${USER}) \
    -f Dockerfile \
    -t fractal/protocol-builder
# Mount entire ./fractal directory so that git works for git revision
# Mount .aws directory so that awscli in download-binaries.sh works
MOUNT_AWS=""
if [[ -d "$HOME/.aws" ]]; then
    MOUNT_AWS="--mount type=bind,source=$HOME/.aws,destination=/home/ubuntu/.aws,readonly"
fi
docker run \
    --rm \
    --env AWS_ACCESS_KEY_ID --env AWS_SECRET_ACCESS_KEY --env AWS_DEFAULT_REGION --env AWS_DEFAULT_OUTPUT \
    --mount type=bind,source=$(cd ..; pwd),destination=/workdir \
    $MOUNT_AWS \
    --name fractal-protocol-builder-$(date +"%s") \
    --user fractal-builder \
    fractal/protocol-builder \
    sh -c "\
    cd protocol &&                 \
    mkdir -p docker-build &&       \
    cd docker-build &&             \
    cmake                          \
        -S ..                      \
        -B .                       \
        ${release_tag} &&          \
    make -j FractalServer          \
    "
