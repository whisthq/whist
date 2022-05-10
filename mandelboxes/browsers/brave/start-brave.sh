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

# Set the Brave language
echo {} | jq '.intl |= . + {"accept_languages": "'"${BROWSER_LANGUAGES}"'", "selected_languages": "'"${BROWSER_LANGUAGES}"'"}' > /home/whist/.config/BraveSoftware/Brave-Browser/Default/Preferences

# Notes on Chromium flags:
#
# The following flags are currently unsupported on Linux, but desirable. Once they are
# supported, we should add support for them:
# "--enable-native-gpu-memory-buffers" --> https://bugs.chromium.org/p/chromium/issues/detail?id=1031269
# "--use-passthrough-command-decoder" --> https://bugs.chromium.org/p/chromium/issues/detail?id=976283
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
# We intentionally disable/don't set the following flags, as they were either causing issues, or
# simply not improving performance:
# "--enable-zero-copy" --> causes visual glitches when using Nvidia Capture
# "--disable-frame-rate-limit" --> significantly degrades frame rate on YouTube
# "--disable-features=UseChromeOSDirectVideoDecoder" --> had no impact
# "--ignore-gpu-blocklist" --> had no impact
# "--disable-software-rasterizer" --> had no impact
# "--ignore-gpu-blocklist" --> had no impact
# "--disable-gpu-vsync" --> had no impact
# "--enable-drdc" --> had no impact
# "--enable-raw-draw" --> not well supported on Linux yet
# "--enable-quic" --> had no impact
# "--enable-lazy-image-loading" --> had no impact
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
  "--enable-gpu-rasterization"
  "--enable-gpu-compositing"
  "--double-buffer-compositing"
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

# OS-specific provisions
# If no CLIENT_OS is passed (i.e. we're testing locally), assume macOS
# and disable smooth scrolling.
if [[ "$CLIENT_OS" == "darwin" || "$CLIENT_OS" == "" ]]; then
  # Edit the Brave Preferences Config file to use the default Mac fonts
  echo {} | \
    jq '.webkit.webprefs.fonts |= . + {"fixed": {"Zyyy": "Courier"}, "sansserif": {"Zyyy": "Helvetica"}, "serif": {"Zyyy": "Times"}, "standard": {"Zyyy": "Times"}}' \
    > /home/whist/.config/BraveSoftware/Brave-Browser/Default/Preferences

  # Disable smooth scrolling, which we handle via uinput instead
  flags+=("--disable-smooth-scrolling")
elif [[ "$CLIENT_OS" == "linux" ]]; then
  # Disable smooth scrolling, which we handle via uinput instead
  flags+=("--disable-smooth-scrolling")
else
  # Edit the Brave Preferences Config file to use the default Windows fonts
  echo {} | \
    jq '.webkit.webprefs.fonts |= . + {"fixed": {"Zyyy": "Consolas"}, "sansserif": {"Zyyy": "Arial"}, "serif": {"Zyyy": "Times New Roman"}, "standard": {"Zyyy": "Times New Roman"}}' \
    > /home/whist/.config/BraveSoftware/Brave-Browser/Default/Preferences
fi

# Load D-Bus configurations; necessary for Brave
# The -10 comes from the display ID
dbus_env_file="/home/whist/.dbus/session-bus/$(cat /etc/machine-id)-10"
# shellcheck source=/dev/null
. "$dbus_env_file"
export DBUS_SESSION_BUS_ADDRESS
echo "loaded d-bus address in start-brave.sh: $DBUS_SESSION_BUS_ADDRESS"

# Start lowercase-files script
# This allows chromium themes to work more consistently
# TODO: Re-enable this once we've found a way for this to enable Chrome themes
# without breaking extension importing
# /usr/bin/lowercase-chromium-files "BraveSoftware" &

# Start Brave with the KDE desktop environment
# flag-switches{begin,end} are no-ops but it's nice convention to use them to surround brave://flags features
exec env XDG_CURRENT_DESKTOP=KDE XDG_SESSION_TYPE=x11 brave-browser "${flags[@]}"
