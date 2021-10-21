#!/bin/bash

# This script is called by the protocol to refresh the DPI of the mandelbox. It first
# writes the DPI to the XResources DB file and to the XSettingsd file. Then, it sends
# SIGHUP to both `xsettingsd` and `awesomewm` in order to refresh both services.

# Exit on subcommand errors
set -Eeuo pipefail

# Takes the DPI as an input, or 192 as the default.
# 96 is a reasonable default, but Macbooks these days
# default to 192, so this is a hack until JSON transport
# is ready!
FRACTAL_DPI=${1:-192}
FRACTAL_DPI_CACHE_FILE=/usr/share/fractal/dpi.cache

# Don't do anything if the DPI didn't change!
if [[ -f "$FRACTAL_DPI_CACHE_FILE" ]]; then
    CACHED_DPI=$(cat $FRACTAL_DPI_CACHE_FILE)
    [[ "$FRACTAL_DPI" == "$CACHED_DPI" ]] && exit
fi

# https://unix.stackexchange.com/a/640599
echo "Xft.dpi: $FRACTAL_DPI" | xrdb -merge
cat << EOF > /home/fractal/.xsettingsd
Xft/DPI $((1024*$FRACTAL_DPI))
Gtk/CursorThemeSize $((24*$FRACTAL_DPI/96))
EOF

echo $FRACTAL_DPI > $FRACTAL_DPI_CACHE_FILE

# Succeed even if these are not yet running, as we have successfully pre-initialized them
killall -HUP xsettingsd awesome || true
