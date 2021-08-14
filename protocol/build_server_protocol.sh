#!/bin/bash

# This script builds the Fractal protocol server for Linux Ubuntu 20.04.

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Set Cmake release flag, if provided. Build in debug mode by default.
if [[ -n "${1:-}" ]]; then
  release_tag="-D CMAKE_BUILD_TYPE=$1"
else
  release_tag="-D CMAKE_BUILD_TYPE=Debug"
fi

# Build protocol
docker build . \
  --build-arg uid=$(id -u ${USER}) \
  -f Dockerfile \
  -t fractal/protocol-builder

DOCKER_USER="fractal-builder"

# We mount .aws directory so that awscli in download-binaries.sh works
MOUNT_AWS=""
if [[ -d "$HOME/.aws" ]]; then
  MOUNT_AWS="--mount type=bind,source=$HOME/.aws,destination=/home/$DOCKER_USER/.aws,readonly"
fi

# We also mount entire ./fractal directory so that git works for git revision
docker run \
  --rm \
  --env AWS_ACCESS_KEY_ID --env AWS_SECRET_ACCESS_KEY --env AWS_DEFAULT_REGION --env AWS_DEFAULT_OUTPUT \
  --mount type=bind,source=$(cd ..; pwd),destination=/workdir \
  $MOUNT_AWS \
  --name fractal-protocol-builder-$(date +"%s") \
  --user "$DOCKER_USER" \
  fractal/protocol-builder \
  sh -c "\
    cd protocol &&                 \
    mkdir -p build-docker &&       \
    cd build-docker &&             \
    cmake                          \
        -S ..                      \
        -B .                       \
        ${release_tag} &&          \
    make -j FractalServer          \
  "
