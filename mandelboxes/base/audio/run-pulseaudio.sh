#!/bin/bash

# This script launches a PulseAudio server associated with the Whist display

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

# Launch PulseAudio
/usr/bin/pulseaudio --system --daemonize=no --exit-idle-time=-1 --disallow-exit
