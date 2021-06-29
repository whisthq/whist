#!/bin/bash

# This script installs dev dependencies needed for Fractal engineer's dev instances

set -Eeuo pipefail

# Retrieve source directory of this script
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# This part sets up the instance, and is necessary for both engineer dev instances and production instances
./setup_ubuntu20_host.sh

# This part installs things that are ONLY necessary for engineer dev instances, NOT for production instances
echo "================================================"
echo "Installing python dependencies..."
echo "================================================"
sudo apt-get install -y python3-pip
cd ..
find ./mandelbox-images -name 'requirements.txt' | sed 's/^/-r /g' | xargs sudo pip3 install
cd ecs-host-setup

echo "================================================"
echo "Creating symlink for root user to get aws credentials..."
echo "================================================"
sudo ln -sf /home/ubuntu/.aws /root/.aws

echo
echo "Install complete. Please 'sudo reboot' before continuing."
echo
