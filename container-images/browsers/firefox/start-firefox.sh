#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Start Firefox with custom profile, set as default browser, and open Google
exec firefox \
        -profile /home/fractal/.mozilla/firefox/fractal_profile \
        -setDefaultBrowser \
        -url "google.com" \
        -MOZ_LOG="PlatformDecoderModule:5" \
        -MOZ_LOG_FILE="/firefox_log.txt"