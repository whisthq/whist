#!/bin/bash

# This script updates the NVIDIA BusID to the current GPU's BusID. It must
# be run before the X Server starts.

# Exit on subcommand errors
set -Eeuo pipefail

# Retrieve the Fractal NVIDIA display config
XCONFIG="/usr/share/X11/xorg.conf.d/01-fractal-nvidia.conf"
if [ ! -f ${XCONFIG} ]; then
    echo "Xconfig at location ${XONFIG} not found (or is not a file)"
    exit 1
fi

# Retrieve the current NVIDIA BusID and the new NVIDIA BusID
OLDBUSID=`awk '/BusID/{gsub(/"/, "", $2); print $2}' ${XCONFIG}`
NEWBUSID=`nvidia-xconfig --query-gpu-info | awk '/PCI BusID/{print $4}'`

# Update the current NVIDIA BusID to the new NVIDIA BusID
if [[ "${OLDBUSID}" == "${NEWBUSID}" ]] ; then
    echo "Nvidia BusID not changed - nothing to do"
else
    echo "Nvidia BusID changed from \"${OLDBUSID}\" to \"${NEWBUSID}\": Updating ${XCONFIG}"
    sed -i -e 's|BusID.*|BusID          '\"${NEWBUSID}\"'|' ${XCONFIG}
fi

# Loop through files /dev/input/eventN to determine which correspond to which device
for filename in /dev/input/event*; do
    name=$(udevadm info -a $filename | grep 'ATTRS{name}' | sed 's/^\s*ATTRS{name}=="\(.*\)"/\1/')
    if [[ $name == 'Fractal Virtual Absolute Input' ]]; then
        echo "Found device file $filename=$name"
        sed -i "s~ABSOLUTE_INPUT_DEVICE~$filename~g" ${XCONFIG}
    fi
    if [[ $name == 'Fractal Virtual Relative Input' ]]; then
        echo "Found device file $filename=$name"
        sed -i "s~RELATIVE_INPUT_DEVICE~$filename~g" ${XCONFIG}
    fi
    if [[ $name == 'Fractal Virtual Keyboard' ]]; then
        echo "Found device file $filename=$name"
        sed -i "s~KEYBOARD_DEVICE~$filename~g" ${XCONFIG}
    fi
done
