#!/bin/bash

# This script is the entrance for Fractal within the Fractal mandelboxes. It retrieves
# the relevant parameters for the mandelbox and starts the fractal systemd user

# Exit on subcommand errors
set -Eeuo pipefail

# Begin wait loop to get TTY number and port mapping from Fractal Host Service
FRACTAL_MAPPINGS_DIR=/fractal/resourceMappings
USER_CONFIGS_DIR=/fractal/userConfigs
APP_CONFIG_MAP_FILENAME=/usr/share/fractal/app-config-map.json

# Wait for TTY and port mapping files and user config to exist
until [ -f $FRACTAL_MAPPINGS_DIR/.ready ]
do
    sleep 0.1
done

# Symlink loaded user configs into the appropriate folders

# While perhaps counterintuitive, "source" is the path in the userConfigs directory
#   and "destination" is the original location of the config file/folder.
#   This is because when creating symlinks, the userConfig path is the source
#   and the original location is the destination
# Iterate through the possible configuration locations and copy
for row in $(cat $APP_CONFIG_MAP_FILENAME | jq -rc '.[]'); do
    SOURCE_CONFIG_SUBPATH=$(echo ${row} | jq -r '.source')
    SOURCE_CONFIG_PATH=$USER_CONFIGS_DIR/$SOURCE_CONFIG_SUBPATH
    DEST_CONFIG_PATH=$(echo ${row} | jq -r '.destination')

    # If original config path does not exist, then continue
    if [ ! -f "$DEST_CONFIG_PATH" ] && [ ! -d "$DEST_CONFIG_PATH" ]; then
        continue
    fi

    # If the source path doesn't exist, then copy default configs to the synced app config folder
    if [ ! -f "$SOURCE_CONFIG_PATH" ] && [ ! -d "$SOURCE_CONFIG_PATH" ]; then
        cp -rT $DEST_CONFIG_PATH $SOURCE_CONFIG_PATH
    fi

    # Remove the original configs and symlink the new ones to the original locations
    rm -rf $DEST_CONFIG_PATH
    ln -sfnT $SOURCE_CONFIG_PATH $DEST_CONFIG_PATH
    chown -R fractal $SOURCE_CONFIG_PATH
done

# Delete broken symlinks from config
find $USER_CONFIGS_DIR -xtype l -delete

# Register TTY once it was assigned via writing to a file by Fractal Host Service
ASSIGNED_TTY=$(cat $FRACTAL_MAPPINGS_DIR/tty)

# Create a TTY within the mandelbox so we don't have to hook it up to one of the host's.
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
