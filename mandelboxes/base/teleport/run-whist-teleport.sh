#!/bin/bash

# This script launches the FUSE filesystem for teleport mounted in the correct directory and
# running in the foreground, so that the PID of the FUSE filesystem is exactly what is returned
# by `getpid()` before the FUSE main function is called.

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
# This is called via `./run-as-whist-user.sh`, which passes sentry environment in.
case $SENTRY_ENVIORNMENT in
  dev|staging|prod)
    eval "$(sentry-cli bash-hook)"
    ;;
  *)
    echo "Sentry environment not set, skipping Sentry error handler"
    ;;
esac

# Exit on subcommand errors
set -Eeuo pipefail

mkdir -p /home/whist/.teleport/drag-drop/fuse
exec /usr/bin/whist-teleport-fuse /home/whist/.teleport/drag-drop/fuse -f
