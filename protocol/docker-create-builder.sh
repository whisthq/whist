#!/bin/bash

# This script creates the Docker container to build the Fractal protocol server on Linux Ubuntu 20.04

# Exit on errors and missing environment variables
set -Eeuo pipefail

docker build . \
             --build-arg uid=$(id -u ${USER}) \
             -f Dockerfile \
             -t fractal/protocol-builder
