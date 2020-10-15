#!/bin/bash
set -e

# Update Nvidia BusID to the current GPU's BusID

XCONFIG="/usr/share/X11/xorg.conf.d/01-fractal-nvidia.conf"

if [ ! -f ${XCONFIG} ]; then
        echo "Xconfig at location ${XONFIG} not found (or is not a file)"
        exit 1
fi

OLDBUSID=`awk '/BusID/{gsub(/"/, "", $2); print $2}' ${XCONFIG}`
NEWBUSID=`nvidia-xconfig --query-gpu-info | awk '/PCI BusID/{print $4}'`

if [[ "${OLDBUSID}" == "${NEWBUSID}" ]] ; then
        echo "Nvidia BUSID not changed - nothing to do"
else
        echo "Nvidia BUSID changed from \"${OLDBUSID}\" to \"${NEWBUSID}\": Updating ${XCONFIG}"
        cp ${XCONFIG} ${XCONFIG}.old
        sed -e 's|BusID.*|BusID          '\"${NEWBUSID}\"'|' ${XCONFIG}.old > ${XCONFIG}
fi
