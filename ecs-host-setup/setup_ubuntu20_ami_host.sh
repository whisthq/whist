#!/bin/bash

# This script takes an EC2 instance already set up for running Fractal manually
# (for development) via setup_ubuntu20_host.sh and sets it up to run Fractal
# automatically (for production).

set -Eeuo pipefail

# Parse input variables
GH_USERNAME=${1}
GH_PAT=${2}
GIT_BRANCH=${3}
GIT_HASH=${4}

# Prevent user from running script as root, to guarantee that all steps are
# associated with the fractal user.
if [ "$EUID" -eq 0 ]; then
    echo "This script cannot be run as root!"
    exit
fi

# Set IP tables for routing networking from host to mandelboxes
sudo sh -c "echo 'net.ipv4.conf.all.route_localnet = 1' >> /etc/sysctl.conf"
sudo sysctl -p /etc/sysctl.conf
echo iptables-persistent iptables-persistent/autosave_v4 boolean true | sudo debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean true | sudo debconf-set-selections

# Disable mandelboxes from accessing the instance metadata service on the host.
# Critical to prevent IAM escalation from within mandelboxes while still using ECS.
sudo iptables -I DOCKER-USER -i docker0 -d 169.254.169.254 -p tcp -m multiport --dports 80,443 -j DROP
sudo iptables -I DOCKER-USER -i docker0 -d 169.254.170.2   -p tcp -m multiport --dports 80,443 -j DROP

sudo sh -c 'iptables-save > /etc/iptables/rules.v4'

# The ECS Host Service gets built in the `fractal-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via ami_config.pkr.hcl
# It gets enabled in base_userdata_template.sh

# Here we pre-pull the desired mandelbox-images onto the AMI to speed up mandelbox startup.
ghcr_uri=ghcr.io
echo "$GH_PAT" | docker login --username "$GH_USERNAME" --password-stdin "$ghcr_uri"
pull_image_base="$ghcr_uri/fractal/$GIT_BRANCH/browsers/chrome"
pull_image="$pull_image_base:$GIT_HASH"
echo "pulling image: $pull_image"
docker pull "$pull_image"

# Tag the image as `current-build` for now as well, so the client app can ask
# for `current-build` without worrying about commit hash mismatches (yet).
# Once maintenance mode removal is solidly implemented, we _shouldn't_ need it,
# but I strongly suspect our deployment pipeline is not up to snuff for us to
# actually get rid of this quite yet.
docker tag "$pull_image" "$pull_image_base:current-build"

echo
echo "Install complete. Make sure you do not reboot when creating the AMI (check NO REBOOT)"
echo
