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

# Launch xinit
nohup /usr/bin/xinit -- /usr/bin/Xorg :${whist_display_number} -noreset +extension GLX +extension RANDR +extension RENDER -logfile ${whist_xorg_log_file} -config ${whist_xorg_conf_file} vt${whist_tty_number} &

# Wait for xinit-pid to be written to, which happens at the bottom of xinitrc
block-until-file-exists.sh /usr/share/whist/xinit-pid >&1

# Exit, to tell systemd that all is well
