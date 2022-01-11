#!/bin/bash

# Used as the entrypoint by Dockerfile

# Exit on subcommand errors
set -Eeuo pipefail

sleep 5 # this fixes an existing race condition between postgres initialization and webserver
waitress-serve --port="$PORT" --url-scheme=https app.entry_web:app
