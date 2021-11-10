#!/bin/bash

# This script spins up the Whist protocol dev client container
# Arguments to this script will be 
# (0) name of this script
# (1) ip address
# (2) port 32262 on the mandelbox
# (3) port 32263 on the mandelbox
# (4) port 32273 on the mandelbox
# (5) AES key
# linux/macos:    ./fclient 35.170.79.124 -p32262:40618.32263:31680.32273:5923 -k 70512c062ff1101f253be70e4cac81bc
# Sample input: ./development/client/spin_up_dev_client_container.sh 35.170.79.124 40618 31680 5923 70512c062ff1101f253be70e4cac81bc

# Exit on subcommand errors
set -Eeuo pipefail

# TODO: 0. Find tag of matching Docker image
## App name will be fractal/development/client:current-build; We need to look for images with a tag that matches either
## one of the following regexes [fractal/development/client:current-build:current-build fractal/development/client:current-build]
DOCKER_IMAGE_NAME="fractal/development/client:current-build"
IMG_LOOKUP_CMD=`docker image inspect ${DOCKER_IMAGE_NAME} >/dev/null 2>&1 && echo yes || echo no`
CMD_OUTPUT="$IMG_LOOKUP_CMD"

if [[ $CMD_OUTPUT == "no" ]]; then
	DOCKER_IMAGE_NAME="fractal/development/client:current-build:current-build"
	IMG_LOOKUP_CMD=`docker image inspect ${DOCKER_IMAGE_NAME} >/dev/null 2>&1 && echo yes || echo no`

	if [[ $CMD_OUTPUT == "no" ]]; then
		echo "Error, development/client image not found!"
		exit
	fi
fi

# TODO: 1. Download any necessary user configs onto the container
# TODO: 2. Apply port bindings/forwarding if needed
# TODO: 3. Pass config variables such as FRACTAL_AES_KEY, which will be saved to file by the startup/entrypoint.sh script, in order for the container to be able to access them later and exported them as environment variables by the `run-fractal-client.sh` script. These config variables will have to be passed as parameters to the FractalClient executable, which will run in non-root mode in the container (username = fractal).
# TODO: 4. Create the Docker container, and start it
docker create -e SERVER_IP_ADDRESS=${1} -e SERVER_PORT_32262=${2} -e SERVER_PORT_32263=${3} -e SERVER_PORT_32273=${4} -e FRACTAL_AES_KEY=${5} $DOCKER_IMAGE_NAME 
docker run $DOCKER_IMAGE_NAME
# TODO: 5. Decrypt user configs within the docker container, if needed
# TODO: 6. Write the config.json file if we want to test JSON transport related features
docker exec $DOCKER_IMAGE_NAME bash -c "touch config.json"
# TODO: 7. Write the .ready file to trigger `base/startup/fractal-startup.sh`
docker exec $DOCKER_IMAGE_NAME bash -c "echo .ready >> .ready"
