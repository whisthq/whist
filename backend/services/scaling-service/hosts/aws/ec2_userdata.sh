#!/bin/bash

# Populate env vars used by this script
USERDATA_ENV=/usr/share/whist/app_env.env
eval "$(cat "$USERDATA_ENV")"

# Note: all commands here are run with the `root` user. It is not necessary to use sudo.
# https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/user-data.html#user-data-shell-scripts
if [ "$EUID" -ne 0 ]; then
  echo "Command should be run as root."
  exit
fi

# Exit on subcommand errors
set -Eeuo pipefail

echo "Whist EC2 userdata started"

####################################################
# Pull Docker images
####################################################

# https://stackoverflow.com/questions/24412721/elegant-solution-to-implement-timeout-for-bash-commands-and-functions/24413646?noredirect=1#24413646
# Usage: run_with_timeout N cmd args...
#    or: run_with_timeout cmd args...
# In the second case, cmd cannot be a number and the timeout will be 10 seconds.
run_with_timeout () {
  local time=10
  if [[ $1 =~ ^[0-9]+$ ]]; then time=$1; shift; fi
  # Run in a subshell to avoid job control messages
  ( "$@" &
    child=$!
    # Avoid default notification in non-interactive shell for SIGTERM
    trap -- "" SIGTERM
    ( sleep $time
    kill $child 2> /dev/null ) &
    wait $child
  )
}

# `retry_docker_pull` will keep retrying until it successfully pulls the
# images.
retry_docker_pull() {
  until pull_docker_images
  do
    echo "Retrying pulling of docker images..."
    sleep 1
  done
}

# `pull_docker_images` contains the commands and setup necessary
#  for pulling the Whist Chrome and Brave Docker images. They will
#  be pulled to the Docker data-root directory.
# Args: none
pull_docker_images() {
  # Login with docker
  echo "$GH_PAT" | docker login ghcr.io -u "$GH_USERNAME" --password-stdin

  # Pull Docker images for Chrome directly to the ephemeral volume
  # Replace `chrome` to pull the image of a different browser.
  pull_image_base_chrome="ghcr.io/whisthq/$GIT_BRANCH/browsers/chrome"
  pull_image_chrome="$pull_image_base_chrome:$GIT_HASH"

  docker pull "$pull_image_chrome"
  docker tag "$pull_image_chrome" "$pull_image_base_chrome:current-build"
}

cd /home/ubuntu

# The first thing we want to do is to set up the ephemeral storage available on
# our instance, if there is some.
EPHEMERAL_DEVICE_PATH=$(nvme list -o json | jq -r '.Devices | map(select(.ModelNumber == "Amazon EC2 NVMe Instance Storage")) | max_by(.PhysicalSize) | .DevicePath')
EPHEMERAL_FS_PATH=/ephemeral
MAX_CONCURRENT_DOWNLOADS=8

# We use ephemeral storage if it exists on our host instances to avoid needing to warm up the filesystem,
# speeding up instance launch time. We move the docker data directory to the ephemeral volume, and then
# pull the Whist docker images.

if [ "$EPHEMERAL_DEVICE_PATH" != "null" ]; then
  echo "Ephemeral device path found: $EPHEMERAL_DEVICE_PATH"

  # Partition ephemeral storage
  parted "$EPHEMERAL_DEVICE_PATH" --script mklabel gpt
  parted "$EPHEMERAL_DEVICE_PATH" --script mkpart primary ext4 2048s 12GiB
  parted "$EPHEMERAL_DEVICE_PATH" --script mkpart primary ext4 12GiB 100%

  # Create an ext4 filesystem for docker using ephemeral
  mkfs -t ext4 "${EPHEMERAL_DEVICE_PATH}p1"
  mkdir -p "$EPHEMERAL_FS_PATH"
  mount "${EPHEMERAL_DEVICE_PATH}p1" "$EPHEMERAL_FS_PATH"

  # Create a swapfile using the other (much larger) partition
  mkswap "${EPHEMERAL_DEVICE_PATH}p2"
  swapon "${EPHEMERAL_DEVICE_PATH}p2"

  # Stop Docker and copy the data directory to the ephemeral storage
  systemctl stop docker
  mv /var/lib/docker "$EPHEMERAL_FS_PATH"
  echo "Moved /var/lib/docker to ephemeral volume"

  # Modify configuration to use the new data directory and persist in daemon config file
  # and start Docker again. Set a higher concurrent download count to speed up the pull.
  jq '. + {"data-root": "'"$EPHEMERAL_FS_PATH/docker"'"}' /etc/docker/daemon.json > tmp.json && mv tmp.json /etc/docker/daemon.json
  jq '. + {"max-concurrent-downloads": '"$MAX_CONCURRENT_DOWNLOADS"'}' /etc/docker/daemon.json > tmp.json && mv tmp.json /etc/docker/daemon.json

  systemctl start docker
else
  echo "No ephemeral device path found. Defaulting to EBS storage."
fi

# Pull Docker images and warmup entire disk in parallel.
run_with_timeout 90 retry_docker_pull &
# Based on initialization commands in https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ebs-initialize.html
# Changes:
#   - changed blocksize to 1M from 128k because optimal dd blocksize is 1M according to above link
fio --filename=/dev/nvme0n1 --rw=read --bs=1M --iodepth=32 --ioengine=libaio --direct=1 --name=volume-initialize &

wait

# The Host Service gets built in the `whist-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via ami_config.json
# Here, we write the systemd unit file for the Whist Host Service.
# Note that we do not restart the host service. This is because if the host
# service dies for some reason, it is not safe to restart it, since we cannot
# tell the status of the existing cloud storage directories. We just need to
# wait for them to unmount (that is an asynchronous process) and the scaling service
# should declare the host dead and prune it.
cat << EOF | sudo tee /etc/systemd/system/host-service.service
[Unit]
Description=Host Service
Requires=docker.service
After=docker.service

[Service]
Restart=no
User=root
Type=exec
EnvironmentFile=/usr/share/whist/app_env.env
ExecStart=/home/ubuntu/host-service

[Install]
WantedBy=multi-user.target
EOF

echo "Created systemd service for host-service"

# Reload daemon files
/bin/systemctl daemon-reload

# Enable Host Service
systemctl enable --now host-service.service

echo "Enabled host-service"

echo "Whist EC2 userdata finished"
