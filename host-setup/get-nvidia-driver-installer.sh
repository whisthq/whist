#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

# Hardcode a GRID driver version to avoid server/client version mismatch.
# This driver version needs to be updated periodically to the latest version
# offered by each cloud provider.
case "$CLOUD_PROVIDER" in
  "AWS")
    curl -o nvidia-driver-installer.run https://ec2-linux-nvidia-drivers.s3.amazonaws.com/grid-14.0/NVIDIA-Linux-x86_64-510.85.02-grid-aws.run
    ;;
  "GCP")
    curl -o nvidia-driver-installer.run https://storage.googleapis.com/nvidia-drivers-us-public/GRID/GRID13.1/NVIDIA-Linux-x86_64-470.82.01-grid.run
    ;;
  *)
    curl -o nvidia-driver-installer.run https://ec2-linux-nvidia-drivers.s3.amazonaws.com/grid-14.0/NVIDIA-Linux-x86_64-510.85.02-grid-aws.run
    ;;
esac

chmod +x ./nvidia-driver-installer.run
