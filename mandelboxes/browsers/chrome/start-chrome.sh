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
# as this prevents Chrome from running forevermore! Hence, we should remove this file when we launch the
# browser for the first time each session. You can think of this as effectively moving the locking mechansim
# out of the backed-up Chrome config folder and into a location that will not persist when the instance dies.
GOOGLE_CHROME_SINGLETON_LOCK=/home/whist/.config/google-chrome/SingletonLock
WHIST_CHROME_SINGLETON_LOCK=/home/whist/.config/WhistChromeSingletonLock

if [[ -n $WHIST_CHROME_SINGLETON_LOCK ]]; then
  touch $WHIST_CHROME_SINGLETON_LOCK
  rm -f $GOOGLE_CHROME_SINGLETON_LOCK
fi

# Notes on Chromium flags:
#
# The following flags are currently unsupported on Linux, but desirable. Once they are
# supported, we should add support for them:
# "--enable-native-gpu-memory-buffers" --> https://bugs.chromium.org/p/chromium/issues/detail?id=1031269
#
# When inspecting chrome://gpu, you'll see that Optimus is disabled. This is intentional, as it
# is not well supported on Linux, and is a feature intended to increase battery life by doing
# process switching, which is undesirable for Whist.
#
# Some of these flags are inspired from the following links:
# https://www.reddit.com/r/Stadia/comments/jwh0sl/greatly_reduce_inputlag_when_playing_in_chrome/
# https://nira.com/chrome-flags/
# https://bbs.archlinux.org/viewtopic.php?id=244031&p=25
#
# We intentionally disable/don't set the following flags, as they were causing issues:
# "--enable-zero-copy" --> causes visual glitches when using Nvidia Capture
# "--disable-frame-rate-limit" --> significantly degrades frame rate on YouTube
#
features="VaapiVideoDecoder,VaapiVideoEncoder,Vulkan,CanvasOopRasterization,OverlayScrollbar,ParallelDownloading"
flags=(
  "--use-gl=desktop"
  "--flag-switches-begin"
  "--enable-show-autofill-signatures"
  "--enable-vp9-kSVC-decode-acceleration"
  "--enable-accelerated-video-decode"
  "--enable-accelerated-mjpeg-decode"
  "--enable-accelerated-2d-canvas"
  "--enable-lazy-image-loading"
  "--enable-gpu-compositing"
  "--enable-gpu-rasterization"
  "--use-passthrough-command-decoder"
  "--double-buffer-compositing"
  "--disable-smooth-scrolling" # We handle smooth scrolling ourselves via uinput
  "--disable-font-subpixel-positioning"
  "--disable-gpu-process-crash-limit"
  "--no-default-browser-check"
  "--ozone-platform-hint=x11"
  "--password-store=basic" # This disables the kwalletd backend, which we don't support
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

# Start Chrome in Kiosk mode (full-screen). This flag is used when the client is a
# local Chromium browser integrating Whist to avoid duplicating the URL bar in the cloud tabs, and should
# not be set when the client is a fully-streamed browser rendered via SDL.
if [[ "$KIOSK_MODE" == true ]]; then
  flags+=("--kiosk")
fi

# Passing the initial url from JSON transport as a parameter to the google-chrome command. If the url is not
# empty, Chrome will open the url as an additional tab at start time. The other tabs will be restored depending
# on the user settings.
flags+=("$INITIAL_URL")

# Load D-Bus configurations; necessary for Chrome
# The -10 comes from the display ID
dbus_env_file="/home/whist/.dbus/session-bus/$(cat /etc/machine-id)-10"
# shellcheck source=/dev/null
. "$dbus_env_file"
export DBUS_SESSION_BUS_ADDRESS
echo "loaded d-bus address in start-chrome.sh: $DBUS_SESSION_BUS_ADDRESS"

# Start lowercase-files script
# This allows chromium themes to work more consistently
/usr/bin/lowercase-chromium-files "google-chrome" &

# Start Chrome with the KDE desktop environment
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround chrome://flags features
exec env XDG_CURRENT_DESKTOP=KDE XDG_SESSION_TYPE=x11 google-chrome "${flags[@]}"
