#!/bin/bash

# This script runs a Fractal container by executing the `docker run` command with the right
# arguments, and also containers helper functions for manually killing a container and for
# manually sending a DPI request to a locally-run container (two tasks handled by the Fractal
# webserver, in production), which facilitates local development.

set -Eeuo pipefail

# Fractal container image to run
image=${1:-fractal/base:current-build}

# Define the folder to mount the Fractal protocol server into the container
if [[ ${2:-''} == mount ]]; then
    mount_protocol="--mount type=bind,source=$(cd base/protocol/server/build64;pwd),destination=/usr/share/fractal/bin"
else
    mount_protocol=""
fi

# DPI to set the X Server to inside the container, defaults to 96 (High Definition) if not set
dpi=${FRACTAL_DPI:-96}

# Run the specified Docker container image with as-restrictive-as-possible system
# capabilities, for security purposes
run_container() {
    docker run -it -d \
        -v /sys/fs/cgroup:/sys/fs/cgroup:ro \
        -v /fractal/containerResourceMappings:/fractal/containerResourceMappings:ro \
        -v /fractal/cloudStorage:/fractal/cloudStorage:rshared \
        -v /tmp/uinput.socket:/tmp/uinput.socket \
        --device=/dev/input/event3 \
        --device=/dev/input/mouse0 \
        --device=/dev/input/event4 \
        $mount_protocol \
        --tmpfs /run \
        --tmpfs /run/lock \
        --gpus all \
        -e NVIDIA_CONTAINER_CAPABILITIES=all \
        -e NVIDIA_VISIBLE_DEVICES=all \
        --shm-size=8g \
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
        -p 32262:32262 \
        -p 32263:32263/udp \
        -p 32273:32273 \
        $image
    # capabilities not enabled by default: CAP_NICE
}

# Helper fucntion to kill a locally running Docker container
# Args: container_id
kill_container() {
  docker kill $1 > /dev/null || true
  docker rm $1 > /dev/null || true
}

# Send a DPI request to the Fractal ECS host service HTTP server running on localhost
# This is necessary for the Fractal server protocol to think that it is ready to start. In production,
# the webserver would send this request to the Fractal host service, but for local development we need
# to send it manually until our development pipeline is fully built
# Args: container_id, DPI
send_dpi_request() {
  # Check if Fractal host service is even running
  sudo lsof -i :4678 | grep ecs-host > /dev/null \
  || (echo "Cannot start container because the ecs-host-service is not listening on port 4678. Is it running successfully?" \
     && kill_container $1 \
     && exit 1)

  # Send the DPI/container-ready request
  response=$(curl --insecure --silent --location --request PUT 'https://localhost:4678/set_container_dpi' \
    --header 'Content-Type: application/json' \
    --data-raw '{
      "auth_secret": "testwebserverauthsecretdev",
      "host_port": 32262,
      "dpi": '"$dpi"'
    }') \
  || (echo "DPI/container-ready request to the host service failed!" \
      && kill_container $1 \
      && exit 1)
  echo "Sent DPI/container-ready request to container $1!"
  echo "Response to DPI/container-ready request from host service: $response"
}

# Main executing thread
# Set the container_id and DPI request
container_id=$(run_container $1)
send_dpi_request $container_id $dpi

# Run the Docker container
echo "Running container with ID: $container_id"
docker exec -it $container_id /bin/bash || true

# Kill the Docker container once we are done
kill_container $container_id
