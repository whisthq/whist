#!/bin/bash

# # Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
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

# # Exit on subcommand errors
set -Eeuo pipefail

# Wait for the PulseAudio unix socket to be active
while ! pactl list | grep UNIX; do
  sleep 0.1
done
