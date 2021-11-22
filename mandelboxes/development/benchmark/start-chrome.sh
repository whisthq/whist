#!/bin/bash

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
  --disable-system-notifications \
  --flag-switches-end \
  /home/fractal/1080p_60fps.html
