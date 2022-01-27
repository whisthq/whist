#!/bin/bash

# This script updates the X11 config file in /usr/share/X11/xorg.conf.d/01-whist-display.conf
# to use the correct Nvidia bus ID and the correct uinput devices for absolute/relative
# mouse input and keyboard input.

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

# Retrieve mandelbox parameters
WHIST_MAPPINGS_DIR=/whist/resourceMappings
GPU_INDEX_FILENAME=gpu_index
GPU_INDEX=$(cat $WHIST_MAPPINGS_DIR/$GPU_INDEX_FILENAME)

echo "Using GPU Index ${GPU_INDEX}"

# Retrieve the Whist NVIDIA display config
XCONFIG="/usr/share/X11/xorg.conf.d/01-whist-display.conf"
if [ ! -f ${XCONFIG} ]; then
  echo "Xconfig at location ${XCONFIG} not found (or is not a file)"
  exit 1
fi

####################
# Update Bus ID
####################

# Retrieve the current NVIDIA BusID and the new NVIDIA BusID
OLDBUSID=`awk '/BusID/{gsub(/"/, "", $2); print $2}' ${XCONFIG}`
# Note that we need to add 1 to GPU_INDEX since `tail` and `head` are 1-indexed.
NEWBUSID=`nvidia-xconfig --query-gpu-info | awk '/PCI BusID/{print $4}' | tail +$(($GPU_INDEX+1)) | head -n1`

# Update the current NVIDIA BusID to the new NVIDIA BusID
if [[ "${OLDBUSID}" == "${NEWBUSID}" ]] ; then
  echo "Nvidia BusID not changed - nothing to do"
else
  echo "Nvidia BusID changed from \"${OLDBUSID}\" to \"${NEWBUSID}\": Updating ${XCONFIG}"
  sed -i -e 's|BusID.*|BusID          '\"${NEWBUSID}\"'|' ${XCONFIG}
fi

############################
# Update uinput devices
############################

# Loop through files /dev/input/eventN to determine which correspond to which device
for filename in /dev/input/event*; do
  name=$(udevadm info -a $filename | grep 'ATTRS{name}' | sed 's/^\s*ATTRS{name}=="\(.*\)"/\1/')
  if [[ $name == 'Whist Virtual Absolute Input' ]]; then
    echo "Found device file $filename=$name"
    sed -i "s~ABSOLUTE_INPUT_DEVICE~$filename~g" ${XCONFIG}
  fi
  if [[ $name == 'Whist Virtual Relative Input' ]]; then
    echo "Found device file $filename=$name"
    sed -i "s~RELATIVE_INPUT_DEVICE~$filename~g" ${XCONFIG}
  fi
  if [[ $name == 'Whist Virtual Keyboard' ]]; then
    echo "Found device file $filename=$name"
    sed -i "s~KEYBOARD_DEVICE~$filename~g" ${XCONFIG}
  fi
done
