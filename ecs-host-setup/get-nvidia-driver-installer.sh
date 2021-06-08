#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/latest/NVIDIA-Linux-x86_64-460.73.01-grid-aws.run nvidia-driver-installer.run
chmod +x ./nvidia-driver-installer.run
