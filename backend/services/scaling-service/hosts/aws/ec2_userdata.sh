#!/bin/bash

# This script contains the configuration for the AWS EC2 instances that get launched to run
# users' cloud browsers. This gets run at launch time of an EC2 instance, by the instance itself.

# Exit on subcommand errors
set -Eeuo pipefail

echo "Whist EC2 userdata started"

cd /home/ubuntu

# The Host-Service gets built in the `whist-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via `ami_config.json``
# Here, we write the systemd unit file for the Whist Host-Service.
# Note that we do not restart the Host-Service. This is because if the Host-Service
# dies for some reason, it is not safe to restart it, since we cannot
# tell the status of the host directories mounted into the container. We just need to
# wait for them to unmount (that is an asynchronous process) and the scaling-service
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

echo "Created systemd service for Host-Service"

# Reload daemon files
sudo /bin/systemctl daemon-reload

# Warm Up EBS Volume, so that disk read/write operations are fast.
# For more information, see: https://github.com/whisthq/whist/pull/5333
sudo fio --filename=/dev/nvme0n1 --rw=read --bs=128k --iodepth=32 --ioengine=libaio --direct=1 --name=volume-initialize

# Enable Host-Service
sudo systemctl enable --now host-service.service

echo "Enabled Host-Service"

echo "Whist EC2 userdata finished"
