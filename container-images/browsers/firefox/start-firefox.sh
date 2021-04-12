#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Start Firefox with custom profile and set as default browser
exec firefox \
        -profile /home/fractal/.mozilla/firefox/fractal_profile \
        -setDefaultBrowser