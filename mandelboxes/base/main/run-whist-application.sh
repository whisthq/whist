#!/bin/bash

# This script starts the symlinked whist-application as the whist user.

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
# This is called via `./run-as-whist-user.sh`, which passes sentry environment in.
case $SENTRY_ENVIRONMENT in
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

# Write the PID to a file
WHIST_APPLICATION_PID_FILE=/home/whist/whist-application-pid
echo $$ > $WHIST_APPLICATION_PID_FILE

# Wait for the PID file to have been removed
block-while-file-exists.sh $WHIST_APPLICATION_PID_FILE >&1

# Pass JSON transport settings as environment variables
# export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
export LOAD_EXTENSION=$LOAD_EXTENSION
export TZ=$TZ # TZ variable automatically adjusts the timezone (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
export INITIAL_URL=$INITIAL_URL
export USER_AGENT=$USER_AGENT
export KIOSK_MODE=$KIOSK_MODE
# export LONGITUDE=$LONGITUDE
# export LATITUDE=$LATITUDE
# Set the locale (the line below exports all the locale environment variables)
# export ${USER_LOCALE?}
# export LANGUAGE=$SYSTEM_LANGUAGES
# export BROWSER_LANGUAGES=$BROWSER_LANGUAGES
export CLIENT_OS=$CLIENT_OS

# Explicitly export the fonts path, so that the
# application can find the fonts. Per: https://askubuntu.com/questions/492033/fontconfig-error-cannot-load-default-config-file
export FONTCONFIG_PATH=/etc/fonts
FONTCONFIG_LOCAL_FOLDER=/home/whist/.config/fontconfig
# Set FONTCONFIG_FILE based on client OS (darwin, win32, linux)
if [[ "$CLIENT_OS" == "darwin" ]]; then
  FONT_CONFIG_CUSTOM_FILE=$FONTCONFIG_LOCAL_FOLDER/mac-fonts.conf
elif [[ "$CLIENT_OS" == "win32" ]]; then
  FONT_CONFIG_CUSTOM_FILE=$FONTCONFIG_LOCAL_FOLDER/windows-fonts.conf
else
  FONT_CONFIG_CUSTOM_FILE=$FONTCONFIG_LOCAL_FOLDER/linux-fonts.conf
fi
cp -f $FONT_CONFIG_CUSTOM_FILE $FONTCONFIG_LOCAL_FOLDER/fonts.conf

# To avoid interfering with Filebeat, the logs files should not contain hyphens in the name before the {-out, -err}.log suffix
APPLICATION_OUT_FILENAME=/home/whist/whist_application-out.log
APPLICATION_ERR_FILENAME=/home/whist/whist_application-err.log

# Start the application that this mandelbox runs
exec whist-application > $APPLICATION_OUT_FILENAME 2>$APPLICATION_ERR_FILENAME
