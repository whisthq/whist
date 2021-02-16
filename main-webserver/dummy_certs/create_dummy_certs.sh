#!/bin/bash

# Exit on errors and missing environment variables
set -Eeuo pipefail

# Retrieve relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Make sure we are always running this script with working directory `main-webserver/dummy_certs`
cd "$DIR"

openssl req  -nodes -new -x509 -subj "/C=US/ST=./L=./O=./CN=." -keyout key.pem -out cert.pem
