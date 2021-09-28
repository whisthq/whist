#!/bin/bash

# Used as the entrypoint by Dockerfile

# TODO use this as the entrypoint for the Procfile as well, e.g.
# Procfile
#   web: waitress-serve --url-scheme=https --port=$PORT entry_web:app
#
# This will ensure increased uniformity between dev and production
# environments.

# Exit on subcommand errors
set -Eeuo pipefail
sleep 5 # this fixes an existing race condition between postgres initialization and web server
waitress-serve --port="$PORT" --url-scheme=https entry_web:app
