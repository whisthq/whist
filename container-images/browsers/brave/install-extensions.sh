#!/bin/bash

# This script defines a general-purpose function to install any Brave Browser extension
# by passing in the extension ID and name

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Brave Browser extension installer
install_brave_extension () {
    # Extensions folder and format
    preferences_dir_path="/opt/brave.com/brave/extensions"
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

# Install uBlock Ad Blocker
install_brave_extension "cjpalhdlnbpafiamejdnhcphjbkeiagm" "uBlock Origin"
