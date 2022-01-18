#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

echo "Whist EC2 userdata started"

cd /home/ubuntu

# The first thing we want to do is to set up the ephemeral storage available on
# our instance, if there is some.
EPHEMERAL_DEVICE_PATH=$(sudo nvme list -o json | jq '.Devices | map(select(.ModelNumber == "Amazon EC2 NVMe Instance Storage")) | max_by(.PhysicalSize) | .DevicePath')
EPHEMERAL_FS_PATH=/ephemeral

if [ $EPHEMERAL_DEVICE_PATH != "null" ] 
then
  echo "Ephemeral device path found: $EPHEMERAL_DEVICE_PATH"
  sudo mkfs -t ext4 "$EPHEMERAL_DEVICE_PATH"
  sudo mkdir -p "$EPHEMERAL_FS_PATH"
  sudo mount "$EPHEMERAL_DEVICE_PATH" "$EPHEMERAL_FS_PATH"
  echo "Mounted ephemeral storage at $EPHEMERAL_FS_PATH"
else
  echo "No ephemeral device path found."
fi





# The Host Service gets built in the `whist-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via ami_config.json
# Here, we write the systemd unit file for the Whist Host Service.
# Note that we do not restart the host service. This is because if the host
# service dies for some reason, it is not safe to restart it, since we cannot
# tell the status of the existing cloud storage directories. We just need to
# wait for them to unmount (that is an asynchronous process) and the webserver
# should declare the host dead and prune it.
sudo cat << EOF > /etc/systemd/system/host-service.service
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
sudo /bin/systemctl daemon-reload

# Enable Host Service
sudo systemctl enable --now host-service.service

echo "Enabled host-service"

echo "Whist EC2 userdata finished"
