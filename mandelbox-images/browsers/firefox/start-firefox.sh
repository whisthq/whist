#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Use X11 (Wayland not supported by native NVIDIA driver yet)
export MOZ_X11_EGL=1

# Start Firefox with custom profile, set as default browser, and open Google
exec firefox \
    -profile /home/fractal/.mozilla/firefox/fractal_profile \
    -setDefaultBrowser \
    -url "google.com"
