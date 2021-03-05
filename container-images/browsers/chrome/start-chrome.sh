#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Load Chrome extensions and start Google Chrome
exec google-chrome --load-extensions=/opt/google/chrome/extensions/cjpalhdlnbpafiamejdnhcphjbkeiagm
