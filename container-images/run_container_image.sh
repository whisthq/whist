#!/bin/bash

# This script runs a Fractal container by executing the `docker run` command with the right
# 


# 



set -Eeuo pipefail

# Fractal container image to run
image=${1:-fractal/base:current-build}

# Define the folder to mount the Fractal protocol server into the container
if [[ ${2:-''} == mount ]]; then
    mount_protocol="--mount type=bind,source=$(cd base/protocol/server/build64;pwd),destination=/usr/share/fractal/bin"
else
    mount_protocol=""
fi

# DPI to set the X Server to inside the container, defaults to 96 (HD) if not set
dpi=${FRACTAL_DPI:-96}







run_container() {
    docker run -it -d \
        -v /sys/fs/cgroup:/sys/fs/cgroup:ro \
        -v /fractal/containerResourceMappings:/fractal/containerResourceMappings:ro \
        -v /fractal/cloudStorage:/fractal/cloudStorage:rshared \
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

# Args: containerid
kill_container() {
  docker kill $1 > /dev/null || true
  docker rm $1 > /dev/null || true
}

# This is necessary for the protocol to think it's ready to start. Normally,
# the webserver would send this request to the host service, but we don't have
# a full development pipeline yet, so this will have to do.
# Args: container_id, DPI
send_dpi_request() {
  # Check if host service is even running
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


container_id=$(run_container $1)
send_dpi_request $container_id $dpi
echo "Running container with ID: $container_id"
docker exec -it $container_id /bin/bash || true
kill_container $container_id
