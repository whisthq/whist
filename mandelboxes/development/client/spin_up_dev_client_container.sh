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

# Ensure that the /fractal folder exists
if [ ! -d "/fractal" ]; then
  echo "/fractal folder does not exist"
  exit 1
fi

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

# Generate string with ENVS
CONTAINER_ENVS="-e SERVER_IP_ADDRESS=${1} \
-e SERVER_PORT_32262=${2} \
-e SERVER_PORT_32263=${3} \
-e SERVER_PORT_32273=${4} \
-e FRACTAL_AES_KEY=${5} \
-e NVIDIA_DRIVER_CAPABILITIES=all \
-e NVIDIA_VISIBLE_DEVICES=all"

random_str_cmd=`echo $RANDOM | md5sum | head -c 20`
random_str="$random_str_cmd"
fake_mandelboxID="dev_client_${random_str}"

# Generate the Resource Mappings folder
mkdir -p /fractal/$fake_mandelboxID/containerResourceMappings

DOCKER_CREATE_CMD=`docker create -it \
    -v "/sys/fs/cgroup:/sys/fs/cgroup:ro" \
    -v "/fractal/$fake_mandelboxID/containerResourceMappings:/fractal/resourceMappings:ro" \
    -v "/fractal/temp/$fake_mandelboxID/sockets:/tmp/sockets" \
    -v "/run/udev/data:/run/udev/data:ro" \
    -v "/fractal/$fake_mandelboxID/userConfigs/unpacked_configs:/fractal/userConfigs:rshared" \
    -v "/fractal/temp/logs/$fake_mandelboxID/$RANDOM:/var/log/fractal" \
    --tmpfs /run \
    --tmpfs /run/lock \
    --gpus all \
    $CONTAINER_ENVS \
    --shm-size=2147483648 \
    --cap-drop ALL \
    --cap-add CAP_SETPCAP \
    --cap-add CAP_MKNOD \
    --cap-add CAP_AUDIT_WRITE \
    --cap-add CAP_CHOWN \
    --cap-add CAP_NET_RAW \
    --cap-add CAP_DAC_OVERRIDE \
    --cap-add CAP_FOWNER \
    --cap-add CAP_FSETID \
    --cap-add CAP_KILL \
    --cap-add CAP_SETGID \
    --cap-add CAP_SETUID \
    --cap-add CAP_NET_BIND_SERVICE \
    --cap-add CAP_SYS_CHROOT \
    --cap-add CAP_SETFCAP \
    --cap-add SYS_NICE \
    $DOCKER_IMAGE_NAME | tail -1`
CONTAINER_ID="$DOCKER_CREATE_CMD"

echo "Created dev client with docker container ID= ${CONTAINER_ID}"

docker start $CONTAINER_ID
# TODO: 6. Write the config.json file if we want to test JSON transport related features
docker exec $CONTAINER_ID /bin/bash -c "touch config.json"
# TODO: 7. Write the .ready file to trigger `base/startup/fractal-startup.sh`
docker exec $CONTAINER_ID /bin/bash -c "echo .ready >> .ready"




