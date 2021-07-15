#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

# Hardcode a GRID driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/g4/grid-11.2/NVIDIA-Linux-x86_64-450.89-grid-aws.run nvidia-driver-installer.run
chmod +x ./nvidia-driver-installer.run
