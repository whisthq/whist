#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

# Parse input variable(s)
CLOUD_PROVIDER=${1}

if [[ "$CLOUD_PROVIDER" == "aws" ]]; then
    echo "Installing Nvidia GRID drivers for AWS..."
    # From AWS: Starting with GRID version 11.0, you can use the driver packages under latest for both G3 and G4dn instances. AWS will not add
    # versions later than 11.0 to g4/latest, but will keep version 11.0 and the earlier versions specific to G4dn under g4/latest.

    # Hardcode a GRID driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
    aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-13.0/NVIDIA-Linux-x86_64-470.63.01-grid-aws.run nvidia-driver-installer.run
    chmod +x ./nvidia-driver-installer.run
elif [[ "$CLOUD_PROVIDER" == "azure" ]]; then
    echo "Installing Nvidia GRID drivers for Azure..."
    # Azure provides GRID drivers for their NV-series instances, with instructions on installing the drivers available 
    # at https://docs.microsoft.com/en-us/azure/virtual-machines/linux/n-series-driver-setup. However, these instructions
    # are out of date...
    # Instead, we install the drivers ourselves from the following sources: https://github.com/Azure/azhpc-extensions/blob/master/NvidiaGPU/resources.json

    # Hardcode a GRID driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
    wget https://download.microsoft.com/download/5/f/9/5f9d855b-8910-4e2d-90a6-5a038e78940f/NVIDIA-Linux-x86_64-460.73.01-grid-azure.run -O nvidia-driver-installer.run
    chmod +x ./nvidia-driver-installer.run
else
    echo "ERROR: Invalid cloud provider: $CLOUD_PROVIDER. This cloud provider is either invalid or not supported yet!"
    exit -1
fi
