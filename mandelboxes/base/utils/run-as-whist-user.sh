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

# Things running outside run_as_whist are run as root.
# Running with login means that we lose all environment variables, so we need to pass them in manually.

# default values for the JSON transport settings from the client
DARK_MODE=false
RESTORE_LAST_SESSION=true
DESIRED_TIMEZONE=Etc/UTC
INITIAL_KEY_REPEAT=68 # default value on macOS, options are 120, 94, 68, 35, 25, 15
KEY_REPEAT=6 # default value on macOS, options are 120, 90, 60, 30, 12, 6, 2
INITIAL_URL=""
USER_AGENT=""

WHIST_JSON_FILE=/whist/resourceMappings/config.json
if [[ -f $WHIST_JSON_FILE ]]; then
  if [ "$( jq -rc 'has("dark_mode")' < $WHIST_JSON_FILE )" == "true"  ]; then
    DARK_MODE="$(jq -rc '.dark_mode' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("restore_last_session")' < $WHIST_JSON_FILE )" == "true"  ]; then
    RESTORE_LAST_SESSION="$(jq -rc '.restore_last_session' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("desired_timezone")' < $WHIST_JSON_FILE )" == "true"  ]; then
    DESIRED_TIMEZONE="$(jq -rc '.desired_timezone' < $WHIST_JSON_FILE)"
    # Set the system-wide timezone
    timedatectl set-timezone "$DESIRED_TIMEZONE"
  fi
  if [ "$( jq -rc 'has("initial_key_repeat")' < $WHIST_JSON_FILE )" == "true"  ]; then
    if [ "$( jq -rc 'has("key_repeat")' < $WHIST_JSON_FILE )" == "true"  ]; then
      INITIAL_KEY_REPEAT=$( jq -rc '.initial_key_repeat' < $WHIST_JSON_FILE )
      KEY_REPEAT=$( jq -rc '.key_repeat' < $WHIST_JSON_FILE )

      # Set the key repeat rates
      xset r rate "$INITIAL_KEY_REPEAT" "$KEY_REPEAT"
    fi
  fi
  if [ "$( jq -rc 'has("initial_url")' < $WHIST_JSON_FILE )" == "true"  ]; then
    INITIAL_URL="$(jq -rc '.initial_url' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("user_agent")' < $WHIST_JSON_FILE )" == "true"  ]; then
    USER_AGENT="$(jq -rc '.user_agent' < $WHIST_JSON_FILE)"
  fi
fi

export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
# Setting the TZ environment variable (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
# in order to automatically adjust the timezone at the lower layers
export TZ=$DESIRED_TIMEZONE
export INITIAL_URL=$INITIAL_URL
export USER_AGENT="$USER_AGENT"
export SENTRY_ENVIRONMENT=${SENTRY_ENVIRONMENT:-}

exec runuser --login whist --whitelist-environment=TZ,DARK_MODE,RESTORE_LAST_SESSION,INITIAL_URL,USER_AGENT,SENTRY_ENVIRONMENT -c \
  'DISPLAY=:10 '\
  'LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 '\
  'LOCAL=yes '\
  'LC_ALL=C '\
  'PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin '\
  'DEBIAN_FRONTEND=noninteractive '"$1"
