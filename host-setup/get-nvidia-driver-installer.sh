#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

case "$CLOUD_PROVIDER" in
    "AWS")
        # From AWS: Starting with GRID version 11.0, you can use the driver packages under latest for both G3 and G4dn instances. AWS will not add
        # versions later than 11.0 to g4/latest, but will keep version 11.0 and the earlier versions specific to G4dn under g4/latest.

        # Hardcode a GRID driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
        aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-14.0/NVIDIA-Linux-x86_64-510.47.03-grid-aws.run nvidia-driver-installer.run
        ;;
    "GCP")
        sudo apt install -y build-essential
        curl -o nvidia-driver-installer.run https://storage.googleapis.com/nvidia-drivers-us-public/GRID/GRID13.0/NVIDIA-Linux-x86_64-470.63.01-grid.run
        ;;
    *)
        aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-14.0/NVIDIA-Linux-x86_64-510.47.03-grid-aws.run nvidia-driver-installer.run
        ;;
esac

chmod +x ./nvidia-driver-installer.run
