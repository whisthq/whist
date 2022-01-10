#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

sudo apt-get update

# Install ffmpeg dependencies
sudo apt-get install --no-install-recommends -y \
  libmfx1 \
  libvo-amrwbenc0 \
  libopencore-amrnb0 \
  libopencore-amrwb0 \
  libx264-155 \
  libx265-179 \
  libopus0 \
  libfdk-aac1 \
  libva-drm2 \
  libvdpau1

# Install Whist dependencies
sudo apt-get install --no-install-recommends -y \
  libssl-dev \
  libgl1-mesa-dri \
  libavcodec58 \
  libavdevice58 \
  libx11-dev \
  libxtst6 \
  xclip \
  x11-xserver-utils \
  libasound2 \
  libxdamage-dev \
  libcurl4-openssl-dev \
  zlib1g
