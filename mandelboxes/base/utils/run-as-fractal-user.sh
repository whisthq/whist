#!/bin/bash

set -Eeuo pipefail

# default values for the JSON transport client settings fron the client
export INITIAL_KEY_REPEAT=68 # default value on macOS, options are 120, 94, 68, 35, 25, 15
export KEY_REPEAT=6 # default value on macOS, options are 120, 90, 60, 30, 12, 6, 2

FRACTAL_JSON_FILE=/fractal/resourceMappings/config.json
if [[ -f $FRACTAL_JSON_FILE ]]; then
  if [ "$( jq 'has("initial_key_repeat")' < $FRACTAL_JSON_FILE )" == "true" && "$( jq 'has("initial_key_repeat")' < $FRACTAL_JSON_FILE )" == "true" ]; then
    INITIAL_KEY_REPEAT=$( jq -r '.initial_key_repeat' < $FRACTAL_JSON_FILE )
    KEY_REPEAT=$( jq -r '.key_repeat' < $FRACTAL_JSON_FILE )

    echo "INITIAL_KEY_REPEAT IS $INITIAL_KEY_REPEAT"
    echo "KEY_REPEAT IS $KEY_REPEAT"

    xset r rate $INITIAL_KEY_REPEAT $KEY_REPEAT
  fi
fi

# Things running outside run_as_fractal are run as root.
# Running with login means that we lose all environment variables, so we need to pass them in manually.
exec runuser --login fractal -c \
  'DISPLAY=:10 \
    LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
    LOCAL=yes \
    LC_ALL=C \
    PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  DEBIAN_FRONTEND=noninteractive '"$1"
