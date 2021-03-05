#!/bin/bash

# This script is the entrance for Fractal within the Fractal Docker containers. It retrieves
# the relevant parameters for the container and starts the fractal systemd user

# Amazon Linux doesn't have the need for a Fractal firewall rule
# yes | ufw allow 5900;

# Allow for SSH login
rm /var/run/nologin
# echo $SSH_PUBLIC_KEY_AWS > ~/.ssh/authorized_keys

# Begin wait loop to get TTY number and port mapping from Fractal ECS host service
FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings

# Wait for TTY and port mapping files to exist
until [ -f $FRACTAL_MAPPINGS_DIR/.ready ]
do
    sleep 0.1
done

# Register TTY once it was assigned via writing to a file by Fractal ECS host service
ASSIGNED_TTY=$(cat $FRACTAL_MAPPINGS_DIR/tty)

# Create a TTY within the container so we don't have to hook it up to one of the host's
# Also, create the device /dev/dri/card0 which is needed for GPU acceleration. Note that
# this CANNOT be done in the Dockerfile because it affects /dev/, so we have to do it here.
# Note that we always use /dev/tty10 even though the minor number below (i.e.
# the number after 4) may change
sudo mknod -m 620 /dev/tty10 c 4 $ASSIGNED_TTY
sudo mkdir /dev/dri
sudo mknod -m 660 /dev/dri/card0 c 226 0

# This installs fractal service
echo "Start Pam Systemd Process for User fractal"
export FRACTAL_UID=`id -u fractal`
systemctl start user@$FRACTAL_UID
