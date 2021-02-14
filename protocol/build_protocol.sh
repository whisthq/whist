#!/bin/bash

# This script builds the Fractal protocol server for Linux Ubuntu 20.04
# This script calls docker-create-builder.sh and docker-run-builder.sh, which build and run the Docker container
# defined by Dockerfile

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Set Cmake release flag
if [[ ${1:-''} == release ]]; then
    release_tag='-D CMAKE_BUILD_TYPE=Release'
else
    release_tag=''
fi

# Build protocol
# Note: we clean build to prevent Cmake caching issues
(cd "$DIR/.." && git clean -dfx -- protocol)
(cd "$DIR" && ./download-binaries.sh)
(cd "$DIR" && ./docker-create-builder.sh)
(cd "$DIR" && ./docker-run-builder-shell.sh \
    $(pwd)/.. \
    "                              \
    cd protocol &&                 \
    cmake                          \
        -S .                       \
        -D BUILD_CLIENT=OFF        \
        -D DOWNLOAD_BINARIES=OFF   \
        ${release_tag} &&          \
    make -j FractalServer          \
    "
)
