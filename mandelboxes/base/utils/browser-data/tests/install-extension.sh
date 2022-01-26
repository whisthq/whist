#!/bin/bash

# This script is used to mock install-extension.sh by
# creating a file with the 1st argument passed in.

# Exit on subcommand errors
set -Eeuo pipefail

touch "$1".json
