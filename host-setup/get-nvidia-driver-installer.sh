#!/bin/bash

# This script retrieves the Nvidia drivers we install on our AWS EC2 instances from AWS S3.

# Exit on subcommand errors
set -Eeuo pipefail

# Which drivers to use:
#
# There are two types of drivers compatible with our system, the GRID drivers and the Gaming drivers
# The Gaming drivers can support more 4K streams concurrently, so we use those. You can find details on
# the differences between the two types of drivers and how to install one or the other here:
# https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/install-nvidia-driver.html
#
# From AWS: Starting with GRID version 11.0, you can use the driver packages under latest for both G3 and G4dn instances. AWS will not add
# versions later than 11.0 to g4/latest, but will keep version 11.0 and the earlier versions specific to G4dn under g4/latest.
#
# From AWS: Regarding the Gaming drivers, you can use the driver packages under linux/latest for both G3 and G4dn instances.

# Hardcode a driver version to avoid server/client version mismatch. This driver version needs to be updated periodically.
# Last updated: December 11, 2021
aws s3 cp --only-show-errors s3://nvidia-gaming/linux/latest/vGPUSW-495.50-Nov2021-vGaming-Linux-Guest-Drivers.zip nvidia-driver-installer.zip

# Place the .run file in the right place and ensure it is executable
unzip nvidia-driver-installer.zip
mv Linux/*.run nvidia-driver-installer.run
chmod +x ./nvidia-driver-installer.run

# Cleanup
rm -rf Linux
rm -rf nvidia-driver-installer.zip
