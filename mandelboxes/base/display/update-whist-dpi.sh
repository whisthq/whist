#!/bin/bash

# This script is called by the protocol to refresh the DPI of the mandelbox. It first
# writes the DPI to the XResources DB file and to the XSettingsd file. Then, it sends
# SIGHUP to both `xsettingsd` and `awesomewm` in order to refresh both services.

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
SENTRY_ENV_FILENAME=/usr/share/whist/private/sentry_env
case $(cat $SENTRY_ENV_FILENAME) in
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

# Obtain the initial DPI from the JSON transport file
WHIST_JSON_FILE=/whist/resourceMappings/config.json
INITIAL_DPI=192
if [[ -f $WHIST_JSON_FILE ]]; then
  if [ "$( jq 'has("client_dpi")' < $WHIST_JSON_FILE )" == "true"  ]; then
    INITIAL_DPI="$(jq -rc '.client_dpi' < $WHIST_JSON_FILE)"
  fi
fi


# Takes the DPI as an input, or the initial value passed by the JSON transport protocol.
# If that's not available, use 192 as the default. 96 is a reasonable default, but
# Macbooks these days default to 192, so this is a hack until JSON transport is ready!
WHIST_DPI=${1:-$INITIAL_DPI}
WHIST_DPI_CACHE_FILE=/usr/share/whist/dpi.cache

# Don't do anything if the DPI didn't change!
if [[ -f "$WHIST_DPI_CACHE_FILE" ]]; then
  CACHED_DPI=$(cat $WHIST_DPI_CACHE_FILE)
  [[ "$WHIST_DPI" == "$CACHED_DPI" ]] && exit
fi

# https://unix.stackexchange.com/a/640599
echo "Xft.dpi: $WHIST_DPI" | xrdb -merge
cat << EOF > /home/whist/.xsettingsd
Xft/DPI $((1024*WHIST_DPI))
Gtk/CursorThemeSize $((24*WHIST_DPI/96))
EOF

echo "$WHIST_DPI" > $WHIST_DPI_CACHE_FILE

# Succeed even if these are not yet running, as we have successfully pre-initialized them
killall -HUP xsettingsd awesome &>/dev/null || true
