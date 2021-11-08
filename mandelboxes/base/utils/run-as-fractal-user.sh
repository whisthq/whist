#!/bin/bash

set -Eeuo pipefail

# Things running outside run_as_fractal are run as root.
# Running with login means that we lose all environment variables, so we need to pass them in manually.

# default values for the JSON transport settings from the client
DARK_MODE=false
RESTORE_LAST_SESSION=true
DESIRED_TIMEZONE=Etc/UTC
INITIAL_KEY_REPEAT=68 # default value on macOS, options are 120, 94, 68, 35, 25, 15
KEY_REPEAT=6 # default value on macOS, options are 120, 90, 60, 30, 12, 6, 2
INITIAL_URL=""

FRACTAL_JSON_FILE=/fractal/resourceMappings/config.json
if [[ -f $FRACTAL_JSON_FILE ]]; then
  if [ "$( jq 'has("dark_mode")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    DARK_MODE="$(jq '.dark_mode' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    DARK_MODE=$(echo $DARK_MODE | tr -d '"')
  fi
  if [ "$( jq 'has("restore_last_session")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    RESTORE_LAST_SESSION="$(jq '.restore_last_session' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    RESTORE_LAST_SESSION=$(echo $RESTORE_LAST_SESSION | tr -d '"')
  fi
  if [ "$( jq 'has("desired_timezone")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    DESIRED_TIMEZONE="$(jq '.desired_timezone' < $FRACTAL_JSON_FILE)"
    # Remove potential quotation marks
    DESIRED_TIMEZONE=$(echo $DESIRED_TIMEZONE | tr -d '"')
    # Set the system-wide timezone
    timedatectl set-timezone $DESIRED_TIMEZONE
  fi
  if [ "$( jq 'has("initial_key_repeat")' < $FRACTAL_JSON_FILE )" == "true"  ]; then 
    if [ "$( jq 'has("key_repeat")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
      INITIAL_KEY_REPEAT=$( jq -r '.initial_key_repeat' < $FRACTAL_JSON_FILE )
      KEY_REPEAT=$( jq -r '.key_repeat' < $FRACTAL_JSON_FILE )

      # Remove potential quotation marks
      INITIAL_KEY_REPEAT=$(echo $INITIAL_KEY_REPEAT | tr -d '"')
      KEY_REPEAT=$(echo $KEY_REPEAT | tr -d '"')

      # Set the key repeat rates
      xset r rate $INITIAL_KEY_REPEAT $KEY_REPEAT
    fi
  fi
  if [ "$( jq 'has("initial_url")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    RECEIVED_URL = "$(jq '.initial_url' < $FRACTAL_JSON_FILE)"
    # Checking for valid URL. https://stackoverflow.com/questions/3183444/check-for-valid-link-url
    regex='(https?|ftp|file)://[-A-Za-z0-9\+&@#/%?=~_|!:,.;]*[-A-Za-z0-9\+&@#/%=~_|]'
    if [[ $RECEIVED_URL =~ $regex ]]; then
      # Remove potential quotation marks
      INITIAL_URL=$(echo $RECEIVED_URL | tr -d '"')
    fi
  fi
fi

export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
# Setting the TZ environment variable (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
# in order to automatically adjust the timezone at the lower layers
export TZ=$DESIRED_TIMEZONE
export INITIAL_URL=$INITIAL_URL

exec runuser --login fractal --whitelist-environment=TZ,DARK_MODE,RESTORE_LAST_SESSION,INITIAL_URL -c \
  'DISPLAY=:10 \
    LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
    LOCAL=yes \
    LC_ALL=C \
    PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  DEBIAN_FRONTEND=noninteractive '"$1"
