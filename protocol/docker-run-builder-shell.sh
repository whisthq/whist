#!/bin/bash

# This script runs the Docker container that builds the Fractal protocol server for Linux Ubuntu 20.04.

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Parameters needed to build the Fractal protocol server in Docker
container_name=fractal-protocol-builder-$(date +"%s") # The name of the Docker container
mount_directory=$(cd ${1:-.}; pwd) # The local directory to mount into the Docker container
username=fractal-builder # The name of the Linux user running the protocol building command
command=${2:-'/bin/bash'} # The command to run to build the Fractal protocol server

if [ -z "$2" ]
then
    docker_run_flags=-it # Flag to open an interactive container instance
fi

# Run the protocol build command in the Docker container with the right parameters
docker run $docker_run_flags \
    --rm \
    --mount type=bind,source=$mount_directory,destination=/workdir \
    --name $container_name \
    --user $username \
    fractal/protocol-builder \
    sh -c "$command"
