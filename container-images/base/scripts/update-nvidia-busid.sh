#!/bin/bash

# This script updates the NVIDIA BusID to the current GPU's BusID. It must
# be run before the X Server starts.

set -e

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
    cp ${XCONFIG} ${XCONFIG}.old
    sed -e 's|BusID.*|BusID          '\"${NEWBUSID}\"'|' ${XCONFIG}.old > ${XCONFIG}
fi
