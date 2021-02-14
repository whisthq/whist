#!/bin/bash

# This script installs the required dependencies to build the Fractal protocol on Linux Ubuntu.

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Install dependencies
sudo apt-get install -y libssl-dev libavcodec-dev libavdevice-dev libx11-dev libxtst-dev xclip x11-xserver-utils libasound2-dev libxdamage-dev libcurl4-openssl-dev zlib1g-dev
