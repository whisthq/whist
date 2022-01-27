#!/bin/bash

# This script is the first systemd service run after systemd starts up. It retrieves
# the relevant parameters for the mandelbox and starts the whist systemd user

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
SENTRY_ENV_FILENAME=/usr/share/whist/private/sentry_env
case $(cat $SENTRY_ENV_FILENAME) in
  dev|staging|prod)
    export SENTRY_ENVIRONMENT=${SENTRY_ENV}
    eval "$(sentry-cli bash-hook)"
    ;;
  *)
    echo "Sentry environment not set, skipping Sentry error handler"
    ;;
esac

# Exit on subcommand errors
set -Eeuo pipefail

# Begin wait loop to get TTY number and port mapping from Whist Host Service
WHIST_MAPPINGS_DIR=/whist/resourceMappings
block-until-file-exists.sh $WHIST_MAPPINGS_DIR/.paramsReady >&1

# Register TTY once it was assigned via writing to a file by Whist Host Service
ASSIGNED_TTY=$(cat $WHIST_MAPPINGS_DIR/tty)

# Create a TTY within the mandelbox so we don't have to hook it up to one of the host's.
# Also, create the device /dev/dri/card0 which is needed for GPU acceleration. Note that
# this CANNOT be done in the Dockerfile because it affects /dev/, so we have to do it here.
# Note that we always use /dev/tty10 even though the minor number below (i.e.
# the number after 4) may change
sudo mknod -m 620 /dev/tty10 c 4 $ASSIGNED_TTY
sudo mkdir /dev/dri
sudo mknod -m 660 /dev/dri/card0 c 226 0

# Set `/var/log/whist` to be root-accessible only
sudo chmod 0600 -R /var/log/whist/

# This installs whist service
echo "Start Pam Systemd Process for User whist"
export WHIST_UID=`id -u whist`
systemctl start user@$WHIST_UID
