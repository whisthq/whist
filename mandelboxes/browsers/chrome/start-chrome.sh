#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Under certain (bad) circumstances, SingletonLock might be saved into the user's config. This is an issue,
# as this prevents Chrome from running forevermore! Hence, we should remove this file when we launch the
# browser for the first time each session. You can think of this as effectively moving the locking mechansim
# out of the backed-up Chrome config folder and into a location that will not persist when the instance dies.
GOOGLE_CHROME_SINGLETON_LOCK=/home/fractal/.config/google-chrome/SingletonLock
FRACTAL_CHROME_SINGLETON_LOCK=/home/fractal/.config/FractalChromeSingletonLock

if [[ ! -f $FRACTAL_CHROME_SINGLETON_LOCK ]]; then
    touch $FRACTAL_CHROME_SINGLETON_LOCK
    rm -f $GOOGLE_CHROME_SINGLETON_LOCK
fi

# Start Chrome
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround chrome://flags features
exec google-chrome \
  --use-gl=desktop \
  --flag-switches-begin \
  --enable-gpu-rasterization \
  --enable-zero-copy \
  --enable-features=VaapiVideoDecoder,Vulkan,CanvasOopRasterization \
  --disable-smooth-scrolling \
  --disable-font-subpixel-positioning \
  --force-color-profile=display-p3-d65 \
  --flag-switches-end \
  --restore-last-session
