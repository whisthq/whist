#!/bin/bash

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
eval "$(sentry-cli bash-hook)"

# Exit on subcommand errors
set -Eeuo pipefail

# Load Chrome extensions and start Google Chrome
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround chrome://flags features
exec google-chrome \
  --use-gl=desktop \
  --load-extensions=/opt/google/chrome/extensions/cjpalhdlnbpafiamejdnhcphjbkeiagm \
  --flag-switches-begin \
  --enable-gpu-rasterization \
  --enable-zero-copy \
  --enable-features=VaapiVideoDecoder,Vulkan \
  --disable-smooth-scrolling \
  --disable-font-subpixel-positioning \
  --flag-switches-end \
  /home/whist/1080p_60fps.html
