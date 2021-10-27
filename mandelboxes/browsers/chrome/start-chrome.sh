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

DARK_MODE=false
RESTORE_LAST_SESSION=true

FRACTAL_JSON_FILE=/fractal/resourceMappings/config.json
if [[ -f $FRACTAL_JSON_FILE ]]; then
  if [ "$( jq 'has("dark_mode")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    DARK_MODE="$(jq '.dark_mode' < $FRACTAL_JSON_FILE)"
  fi
  if [ "$( jq 'has("restore_last_session")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    RESTORE_LAST_SESSION="$(jq '.restore_last_session' < $FRACTAL_JSON_FILE)"
  fi
fi


flags=(
  --use-gl=desktop
  --flag-switches-begin
  --enable-gpu-rasterization
  --enable-zero-copy
  --enable-features=VaapiVideoDecoder,Vulkan
  --disable-smooth-scrolling
  --disable-font-subpixel-positioning
  --force-color-profile=display-p3-d65
  --flag-switches-end
)

if [[ $DARK_MODE == true ]]; then
    flags+=(--enable-features=WebUIDarkMode)
    flags+=(--force-dark-mode)
fi

if [[ $RESTORE_LAST_SESSION == true ]]; then
    flags+=(--restore-last-session)
fi


# Start Chrome
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround chrome://flags features
exec google-chrome "${flags[@]}"
