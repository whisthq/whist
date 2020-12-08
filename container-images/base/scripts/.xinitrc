#!/usr/bin/env bash

# things running outside runuser are run as root
i3 &

CONTAINER_ID=$(basename $(cat /proc/1/cpuset))
DPI_FILENAME=/fractal/containerResourceMappings/$CONTAINER_ID/DPI
# send in dpi if set
if [ -f "$DPI_FILENAME" ]; then
    export FRACTAL_DPI=$(cat $DPI_FILENAME)
    echo "Xft.dpi: $FRACTAL_DPI" | xrdb -merge
fi

# running with login means that we lose all environment variables, so we need to pass them in manually
exec runuser --login fractal -c \
    'DISPLAY=:10 \
	LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
	LOCAL=yes \
	LC_ALL=C \
	PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
	DEBIAN_FRONTEND=noninteractive \
    fractal-application'

