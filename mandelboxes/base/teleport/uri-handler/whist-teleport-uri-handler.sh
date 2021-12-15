#!/bin/bash

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

# Only write if there isn't a URL currently being handled
# Note that writes of size 4096 and under are atomic on
# Linux, so the only edge case is if there's a really long
# URL, in which case it's possible for it to be truncated.
if [[ ! -f /home/whist/.teleport/handled-uri ]]; then
  echo -n "$1" > /home/whist/.teleport/handled-uri
fi
