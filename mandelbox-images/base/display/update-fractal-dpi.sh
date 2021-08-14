#!/bin/bash

# This script is called by the protocol to refresh the DPI of the mandelbox. It first
# writes the DPI to the XResources DB file and to the XSettingsd file. Then, it sends
# SIGHUP to both `xsettingsd` and `awesomewm` in order to refresh both services.

exit 0

# Takes the DPI as an input, or 96 as the default.
FRACTAL_DPI=${1:-96}

# Exit on subcommand errors
set -Eeuo pipefail

# https://unix.stackexchange.com/a/640599
echo "Xft.dpi: $FRACTAL_DPI" | xrdb -merge
cat << EOF > /home/fractal/.xsettingsd
Xft/DPI $((1024*$FRACTAL_DPI))
Gtk/CursorThemeSize $((24*$FRACTAL_DPI/96))
EOF

# Succeed even if these are not yet running, as we have successfully pre-initialized them
killall -HUP xsettingsd awesome || true
