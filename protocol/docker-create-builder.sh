#!/bin/bash
# This script creates the Docker container to build the Fractal protocol
docker build . \
             --build-arg uid=$(id -u ${USER}) \
             -f Dockerfile \
             -t fractal/protocol-builder
