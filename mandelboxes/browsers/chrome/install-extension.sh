#!/bin/bash

# This script defines a general-purpose function to install any Google Chrome extension
# by passing in the extension ID and name

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

# Google Chrome extension installer
install_chrome_extension () {
  # Extensions folder and format
  preferences_dir_path="/opt/google/chrome/extensions"
  pref_file_path="$preferences_dir_path/$1.json"

  # Extension external URL
  upd_url="https://clients2.google.com/service/update2/crx"

  # Create extensions folder structure and retrieve extension
  mkdir -p "$preferences_dir_path"
  echo "{" > "$pref_file_path"
  echo "\"external_update_url\": \"$upd_url\"" >> "$pref_file_path"
  echo "}" >> "$pref_file_path"
  echo Added \""$pref_file_path"\"
}

# Allow developers to install extensions by calling this function
if [ "$#" -eq 1 ]; then
  install_chrome_extension "$1"
else
  echo "Could not install chrome extension $1. Expected 1 arg, got $#"
fi

# Install Chrome (Chromium) Extensions
# format: install_chrome_extension [extension string ID]
# i.e.: install_chrome_extension "cjpalhdlnbpafiamejdnhcphjbkeiagm"
