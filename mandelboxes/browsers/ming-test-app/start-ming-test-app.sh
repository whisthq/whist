#!/bin/bash

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
# This is called via `./run-as-whist-user.sh`, which passes sentry environment in.
case $(cat $SENTRY_ENVIRONMENT) in
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

# Close all instances of Google Chrome (to simplify login redirection from the browser to ming-test-app)
if pgrep chrome; then
    pkill chrome
fi

# Start ming-test-app
exec /usr/bin/Fractal.deb $@
