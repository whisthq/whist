#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

export MOZ_X11_EGL=1

# Start Firefox with custom profile, set as default browser, and open Google
exec firefox \
        -profile /home/fractal/.mozilla/firefox/fractal_profile \
        -setDefaultBrowser \
        -url "google.com" \
        -MOZ_LOG="PlatformDecoderModule:5" \
        -MOZ_LOG_FILE="/home/fractal/firefox_log.txt"