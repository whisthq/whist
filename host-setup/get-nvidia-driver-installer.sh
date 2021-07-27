#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

# From AWS: Starting with GRID version 11.0, you can use the driver packages under latest for both G3 and G4dn instances. We will not add
# versions later than 11.0 to g4/latest, but will keep version 11.0 and the earlier versions specific to G4dn under g4/latest.

# Hardcode a GRID driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-12.2/NVIDIA-Linux-x86_64-460.73.01-grid-aws.run nvidia-driver-installer.run
chmod +x ./nvidia-driver-installer.run
