#!/bin/bash

# This script runs a Fractal container by executing the `docker run` command with the right
# arguments, and also containers helper functions for manually killing a container and for
# manually sending a DPI request to a locally-run container (two tasks handled by the Fractal
# webserver, in production), which facilitates local development.

set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Working directory is fractal/container-images/helper-scripts/

# Set working directory to fractal/container-images
cd "$DIR/.."

# Fractal container image to run
image=${1:-fractal/base:current-build}
app_name=$(echo $image | sed 's/.*fractal\/\(.*\):.*/\1/')

# Define the folder to mount the Fractal protocol server into the container
if [[ "$2" == mount ]]; then
    mount_protocol="--mount type=bind,source=$(cd ../protocol/docker-build/server/build64; pwd),destination=/usr/share/fractal/bin"
else
    mount_protocol=""
fi

# DPI to set the X Server to inside the container, defaults to 96 (High Definition) if not set
dpi=${FRACTAL_DPI:-96}

# User ID to use for user config retrieval
# TODO: This flow will have to change when we actually encrypt the configs.
# (https://github.com/fractal/fractal/issues/1136)
user_id=${FRACTAL_USER_ID:-''}


# Send a start values request to the Fractal ECS host service HTTP server running on localhost
# This is necessary for the Fractal server protocol to think that it is ready to start. In production,
# the webserver would send this request to the Fractal host service, but for local development we need
# to send it manually until our development pipeline is fully built
# Args: DPI, user_id
send_start_values_request() {
    # Send the DPI/container-ready request
    response=$(curl --insecure --silent --location --request PUT 'https://localhost:4678/set_container_start_values' \
            --header 'Content-Type: application/json' \
            --data-raw '{
      "auth_secret": "testwebserverauthsecretdev",
      "host_port": 32262,
      "dpi": '"$1"',
      "user_id": "'"${2:-}"'"
    }') \
        || (echo "DPI/container-ready request to the host service failed!" && killall ecs-host-service && exit 1)
    echo "Sent DPI/container-ready request!"
    echo "Response to DPI/container-ready request from host service: $response"
}


# Main executing thread

# Wait some time for host service to start, then send start values request
(sleep 3 && send_start_values_request $dpi $user_id) &

# Start host service with appropriate environment variables
(cd ../ecs-host-service && APP_NAME="$app_name" APP_IMAGE="$image" MOUNT_COMMAND="$mount_protocol" PRINT_DOCKER_CREATE_COMMAND="true" make run > .host_service_output)


# Run bash inside the Docker container
# docker exec -it $container_id /bin/bash || true

# Kill the container and host service we exit the run bash
# killall ecs-host-service
