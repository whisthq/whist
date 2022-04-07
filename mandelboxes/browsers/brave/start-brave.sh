#!/bin/bash

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
# This is called via `./run-as-whist-user.sh`, which passes sentry environment in.
case $(cat "$SENTRY_ENVIRONMENT") in
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
# as this prevents Brave from running forevermore! Hence, we should remove this file when we launch the
# browser for the first time each session. You can think of this as effectively moving the locking mechansim
# out of the backed-up Brave config folder and into a location that will not persist when the instance dies.
BRAVE_SINGLETON_LOCK=/home/whist/.config/BraveSoftware/Brave-Browser/SingletonLock
WHIST_BRAVE_SINGLETON_LOCK=/home/whist/.config/WhistBraveSingletonLock

if [[ ! -f $WHIST_BRAVE_SINGLETON_LOCK ]]; then
  touch $WHIST_BRAVE_SINGLETON_LOCK
  rm -f $BRAVE_SINGLETON_LOCK
fi

features="VaapiVideoDecoder,Vulkan,CanvasOopRasterization,OverlayScrollbar"
flags=(
  "--use-gl=desktop"
  "--flag-switches-begin"
  "--enable-gpu-rasterization"
  "--enable-zero-copy"
  "--double-buffer-compositing"
  "--disable-smooth-scrolling" # We handle smooth scrolling ourselves via uinput
  "--disable-font-subpixel-positioning"
  "--force-color-profile=display-p3-d65"
  "--disable-gpu-process-crash-limit"
  "--no-default-browser-check"
  "--load-extension=/opt/teleport/chrome-extension"
)

if [[ "$DARK_MODE" == true ]]; then
  features="$features,WebUIDarkMode"
  flags+=("--force-dark-mode")
fi

if [[ "$RESTORE_LAST_SESSION" == true ]]; then
  flags+=("--restore-last-session")
fi

flags+=("--enable-features=$features")
flags+=("--flag-switches-end")

# Pass user agent corresponding to user's OS from JSON-transport
if [[ -n "$USER_AGENT" ]]; then
  flags+=("--user-agent=$USER_AGENT")
fi

# Start Brave in Kiosk mode (full-screen). This flag is used when the client is a
# local Chromium browser integrating Whist to avoid duplicating the URL bar in the cloud tabs, and should
# not be set when the client is a fully-streamed browser rendered via SDL.
if [[ "$KIOSK_MODE" == true ]]; then
  flags+=("--kiosk")
fi

# Passing the initial url from json transport as a parameter to the brave-browser command. If the url is not
# empty, Brave will open the url as an additional tab at start time. The other tabs will be restored depending
# on the user settings.
flags+=("$INITIAL_URL")

# Load D-Bus configurations; necessary for Brave
# The -10 comes from the display ID
dbus_env_file="/home/whist/.dbus/session-bus/$(cat /etc/machine-id)-10"
# shellcheck source=/dev/null
. "$dbus_env_file"
export DBUS_SESSION_BUS_ADDRESS
echo "loaded d-bus address in start-brave.sh: $DBUS_SESSION_BUS_ADDRESS"

# Start Brave with the KDE desktop environment
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround brave://flags features
exec env XDG_CURRENT_DESKTOP=KDE brave-browser "${flags[@]}"
