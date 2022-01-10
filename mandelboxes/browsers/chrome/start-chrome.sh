#!/bin/bash

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
# This is called via `./run-as-whist-user.sh`, which passes sentry environment in.
case $(cat $SENTRY_ENVIRONMENT) in
  dev|staging|prod)
    export SENTRY_ENVIRONMENT=${SENTRY_ENV}
    eval "$(sentry-cli bash-hook)"
    ;;
  *)
    echo "Sentry environment not set, skipping Sentry error handler"
    ;;
esac

# Exit on subcommand errors
set -Eeuo pipefail

# Under certain (bad) circumstances, SingletonLock might be saved into the user's config. This is an issue,
# as this prevents Chrome from running forevermore! Hence, we should remove this file when we launch the
# browser for the first time each session. You can think of this as effectively moving the locking mechansim
# out of the backed-up Chrome config folder and into a location that will not persist when the instance dies.
GOOGLE_CHROME_SINGLETON_LOCK=/home/whist/.config/google-chrome/SingletonLock
WHIST_CHROME_SINGLETON_LOCK=/home/whist/.config/WhistChromeSingletonLock

if [[ ! -f $WHIST_CHROME_SINGLETON_LOCK ]]; then
  touch $WHIST_CHROME_SINGLETON_LOCK
  rm -f $GOOGLE_CHROME_SINGLETON_LOCK
fi

features="VaapiVideoDecoder,Vulkan,CanvasOopRasterization,OverlayScrollbar"
flags=(
  --use-gl=desktop
  --flag-switches-begin
  --enable-gpu-rasterization
  --enable-zero-copy
  --double-buffer-compositing
  --disable-smooth-scrolling
  --disable-font-subpixel-positioning
  --force-color-profile=display-p3-d65
  --disable-gpu-process-crash-limit
  --no-default-browser-check
  --load-extension=/opt/teleport/chrome-extension
)

if [[ $DARK_MODE == true ]]; then
  features="$features,WebUIDarkMode"
  flags+=(--force-dark-mode)
fi

if [[ $RESTORE_LAST_SESSION == true ]]; then
  flags+=(--restore-last-session)
fi

flags+=(--enable-features=$features)
flags+=(--flag-switches-end)

# Pass user agent corresponding to user's OS from JSON-transport
if [[ $USER_AGENT -ne "" ]]; then
  flags+=(--user-agent="$USER_AGENT")
fi

# Passing the initial url from json transport as a parameter to the google-chrome command. If the url is not
# empty, Chrome will open the url as an additional tab at start time. The other tabs will be restored depending
# on the user settings.
flags+=($INITIAL_URL)

# Load D-Bus configurations from .xinitrc
eval `cat /whist/dbus_config.txt`
echo "d-bus address: $DBUS_SESSION_BUS_ADDRESS | pid: $DBUS_SESSION_BUS_PID"

# Start Chrome
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround chrome://flags features
DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS DBUS_SESSION_BUS_PID=$DBUS_SESSION_BUS_PID exec google-chrome "${flags[@]}"
