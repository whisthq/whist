#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

# Hardcode a GRID driver version to avoid server/client version mismatch.
# This driver version needs to be updated periodically to the latest version
# supported by all cloud providers.
VERSION="13.1"
BUILD="NVIDIA-Linux-x86_64-470.82.01-grid"
case "$CLOUD_PROVIDER" in
  "AWS")

    aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-"$VERSION"/"$BUILD"-aws.run
    ;;
  "GCP")
    sudo apt install -y build-essential
    curl -o nvidia-driver-installer.run https://storage.googleapis.com/nvidia-drivers-us-public/GRID/GRID"$VERSION"/"$BUILD".run
    ;;
  *)
    aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-"$VERSION"/"$BUILD"-aws.run
    ;;
esac

chmod +x ./nvidia-driver-installer.run
