#!/bin/bash

# This script installs dev dependencies needed for Fractal engineer's dev instances

set -Eeuo pipefail

# This part installs things that are ONLY necessary for engineer dev instances, NOT for production instances
sudo apt-get install -y python3-pip
cd ..
find ./container-images -name 'requirements.txt' | sed 's/^/-r /g' | xargs sudo pip3 install
cd ecs-host-setup

# This part sets up the instance, and is necessary for both engineer dev instances and production instances
./setup_ubuntu20_host.sh
