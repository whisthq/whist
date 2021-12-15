#!/bin/bash

# This script launches the FUSE filesystem for teleport mounted in the correct directory and
# running in the foreground, so that the PID of the FUSE filesystem is exactly what is returned
# by `getpid()` before the FUSE main function is called.

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

mkdir -p /home/whist/.teleport/drag-drop/fuse
exec /usr/bin/whist-teleport-fuse /home/whist/.teleport/drag-drop/fuse -f
