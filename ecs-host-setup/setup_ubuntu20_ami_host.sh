#!/bin/bash

# This script takes an EC2 instance already set up for running Fractal manually
# (for development) via setup_ubuntu20_host.sh and sets it up to run Fractal
# automatically (for production).

set -Eeuo pipefail

# Parse input variables
GH_PAT={1}
GH_USERNAME={2} 
GIT_BRANCH={3}
GIT_HASH={4}

# Prevent user from running script as root, to guarantee that all steps are
# associated with the fractal user.
if [ "$EUID" -eq 0 ]; then
    echo "This script cannot be run as root!"
    exit
fi

# Create directories for ECS agent
sudo mkdir -p /var/log/ecs /var/lib/ecs/{data,gpu} /etc/ecs

# Install jq to build JSON
sudo apt-get install -y jq

# Create list of GPU devices for mounting to containers
DEVICES=""
for DEVICE_INDEX in {0..64}
do
    DEVICE_PATH="/dev/nvidia${DEVICE_INDEX}"
    if [ -e "$DEVICE_PATH" ]; then
        DEVICES="${DEVICES} --device ${DEVICE_PATH}:${DEVICE_PATH} "
    fi
done
DEVICE_MOUNTS=`printf "$DEVICES"`

# Set IP tables for routing networking from host to containers
sudo sh -c "echo 'net.ipv4.conf.all.route_localnet = 1' >> /etc/sysctl.conf"
sudo sysctl -p /etc/sysctl.conf
echo iptables-persistent iptables-persistent/autosave_v4 boolean true | sudo debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean true | sudo debconf-set-selections
sudo apt-get install -y iptables-persistent
sudo iptables -t nat -A PREROUTING -p tcp -d 169.254.170.2 --dport 80 -j DNAT --to-destination 127.0.0.1:51679
sudo iptables -t nat -A OUTPUT -d 169.254.170.2 -p tcp -m tcp --dport 80 -j REDIRECT --to-ports 51679
sudo sh -c 'iptables-save > /etc/iptables/rules.v4'

# Remove extra unnecessary files
sudo rm -rf /var/lib/cloud/instances/
sudo rm -f /var/lib/ecs/data/*

# The ECS Host Service gets built in the `fractal-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via ami_config.json
# It gets enabled in base_userdata_template.sh

# Here we pre-pull the desired container-images onto the AMI to speed up container startup.
ghcr_uri=ghcr.io
echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri
pull_image="$ghcr_uri/fractal/$GIT_BRANCH/browsers/chrome:$GIT_HASH"
echo "pulling image: $pull_image"
docker pull "$pull_image"

echo
echo "Install complete. Make sure you do not reboot when creating the AMI (check NO REBOOT)"
echo
