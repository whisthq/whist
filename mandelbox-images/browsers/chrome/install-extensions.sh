#!/bin/bash

# This script defines a general-purpose function to install any Google Chrome extension
# by passing in the extension ID and name

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
  echo Added \""$pref_file_path"\" ["$2"]
}

# Install Chromium Extension
# format: install_chrome_extension [extension string ID] [extension name]
# i.e.: install_chrome_extension "cjpalhdlnbpafiamejdnhcphjbkeiagm" "uBlock Origin"
