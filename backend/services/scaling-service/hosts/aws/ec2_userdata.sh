#!/bin/bash

# Note: all commands here are run with the `root` user. It is not necessary to use sudo.
# https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/user-data.html#user-data-shell-scripts
if [ "$EUID" -ne 0 ]; then
  echo "Command should be run as root."
  exit
fi

# Exit on subcommand errors
set -Eeuo pipefail

echo "Whist EC2 userdata started"

cd /home/ubuntu

# The first thing we want to do is to set up the ephemeral storage available on
# our instance, if there is some.
EPHEMERAL_DEVICE_PATH=$(nvme list -o json | jq -r '.Devices | map(select(.ModelNumber == "Amazon EC2 NVMe Instance Storage")) | max_by(.PhysicalSize) | .DevicePath')
EPHEMERAL_FS_PATH=/ephemeral
MAX_CONCURRENT_DOWNLOADS=8
USERDATA_ENV=/usr/share/whist/app_env.env

# We use ephemeral storage if it exists on our host instances to avoid needing to warm up the filesystem,
# speeding up instance launch time. We move the docker data directory to the ephemeral volume, and then
# pull the Whist docker images.

if [ "$EPHEMERAL_DEVICE_PATH" != "null" ]
then
  echo "Ephemeral device path found: $EPHEMERAL_DEVICE_PATH"

  mkfs -t ext4 "$EPHEMERAL_DEVICE_PATH"
  mkdir -p "$EPHEMERAL_FS_PATH"
  mount "$EPHEMERAL_DEVICE_PATH" "$EPHEMERAL_FS_PATH"
  echo "Mounted ephemeral storage at $EPHEMERAL_FS_PATH"

  # Stop docker and copy the data directory to the ephemeral storage
  systemctl stop docker
  mv /var/lib/docker "$EPHEMERAL_FS_PATH"
  echo "Moved /var/lib/docker to ephemeral volume"

  # Modify configuration to use the new data directory and persist in daemon config file
  # and start docker again. Set a higher concurrent download count to speed up the pull.
  jq '. + {"data-root": "'"$EPHEMERAL_FS_PATH/docker"'"}' /etc/docker/daemon.json > tmp.json && mv tmp.json /etc/docker/daemon.json
  jq '. + {"max-concurrent-downloads": '"$MAX_CONCURRENT_DOWNLOADS"'}' /etc/docker/daemon.json > tmp.json && mv tmp.json /etc/docker/daemon.json

  systemctl start docker
else
  echo "No ephemeral device path found. Warming up EBS volume with fio."
  # Warm Up EBS Volume
  # For more information, see: https://github.com/whisthq/whist/pull/5333
  fio --filename=/dev/nvme0n1 --rw=read --bs=128k --iodepth=32 --ioengine=libaio --direct=1 --name=volume-initialize
fi

# Pull the images regardless if the instance has ephemeral storage or not.
# This way we avoid ending with an instance that can't launch mandelboxes.

# Populate env vars
eval "$(cat $USERDATA_ENV)"

# Login with docker
echo "$GH_PAT" | docker login ghcr.io -u "$GH_USERNAME" --password-stdin

# Pull Docker images for Chrome and Brave directly to the ephemeral volume
pull_image_base_chrome="ghcr.io/whisthq/$GIT_BRANCH/browsers/chrome"
pull_image_chrome="$pull_image_base_chrome:$GIT_HASH"

pull_image_base_brave="ghcr.io/whisthq/$GIT_BRANCH/browsers/brave"
pull_image_brave="$pull_image_base_brave:$GIT_HASH"

docker pull "$pull_image_chrome"
docker tag "$pull_image_chrome" "$pull_image_base_chrome:current-build"

docker pull "$pull_image_brave"
docker tag "$pull_image_brave" "$pull_image_base_brave:current-build"

echo "Finished pulling images"

# The Host Service gets built in the `whist-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via ami_config.json
# Here, we write the systemd unit file for the Whist Host Service.
# Note that we do not restart the host service. This is because if the host
# service dies for some reason, it is not safe to restart it, since we cannot
# tell the status of the existing cloud storage directories. We just need to
# wait for them to unmount (that is an asynchronous process) and the webserver
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
