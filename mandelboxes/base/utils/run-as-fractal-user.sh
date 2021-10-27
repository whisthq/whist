#!/bin/bash

set -Eeuo pipefail

# Things running outside run_as_fractal are run as root.
# Running with login means that we lose all environment variables, so we need to pass them in manually.

DARK_MODE=true
RESTORE_LAST_SESSION=true
DESIRED_TIMEZONE="Etc/UTC"

FRACTAL_JSON_FILE=/fractal/resourceMappings/config.json
if [[ -f $FRACTAL_JSON_FILE ]]; then
  if [ "$( jq 'has("dark_mode")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    DARK_MODE="$(jq '.dark_mode' < $FRACTAL_JSON_FILE)"
  fi
  if [ "$( jq 'has("restore_last_session")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    RESTORE_LAST_SESSION="$(jq '.restore_last_session' < $FRACTAL_JSON_FILE)"
  fi
  if [ "$( jq 'has("desired_timezone")' < $FRACTAL_JSON_FILE )" == "true"  ]; then
    DESIRED_TIMEZONE="$(jq '.desired_timezone' < $FRACTAL_JSON_FILE)"
    timedatectl set-timezone $DESIRED_TIMEZONE
  fi
fi

export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
export DESIRED_TIMEZONE=$DESIRED_TIMEZONE

exec runuser --login fractal --whitelist-environment=DESIRED_TIMEZONE,DARK_MODE,RESTORE_LAST_SESSION -c \
  'DISPLAY=:10 \
    LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
    LOCAL=yes \
    LC_ALL=C \
    PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  DEBIAN_FRONTEND=noninteractive '"$1"
