#!/bin/bash

# This script retrieves the Nvidia GRID drivers from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail






# From AWS: Starting with GRID version 11.0, you can use the driver packages under latest for both G3 and G4dn instances. AWS will not add
# versions later than 11.0 to g4/latest, but will keep version 11.0 and the earlier versions specific to G4dn under g4/latest.

# Hardcode a GRID driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
aws s3 cp --only-show-errors s3://ec2-linux-nvidia-drivers/grid-13.0/NVIDIA-Linux-x86_64-470.63.01-grid-aws.run nvidia-driver-installer.run
chmod +x ./nvidia-driver-installer.run




# From Azure: https://developer.download.nvidia.com/compute/cuda/repos/
# https://github.com/Azure/azhpc-extensions/blob/master/NvidiaGPU/resources.json

wget https://download.microsoft.com/download/5/f/9/5f9d855b-8910-4e2d-90a6-5a038e78940f/NVIDIA-Linux-x86_64-460.73.01-grid-azure.run -O nvidia-driver-installer.run
chmod +x ./nvidia-driver-installer.run


# verify that the system has a CUDA-capable GPU
# lspci | grep -i NVIDIA

# CUDA_REPO_PKG=cuda-drivers_470.82.01-1_amd64.deb
# wget -O /tmp/${CUDA_REPO_PKG} https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/${CUDA_REPO_PKG} 

# sudo dpkg -i /tmp/${CUDA_REPO_PKG}
# sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/7fa2af80.pub 
# rm -f /tmp/${CUDA_REPO_PKG}

# sudo apt-get update
# sudo apt-get install cuda-drivers

