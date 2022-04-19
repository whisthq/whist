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
LATITUDE=""
LONGITUDE=""
USER_LOCALE=""
SYSTEM_LANGUAGES="en_US"
BROWSER_LANGUAGES="en-US,en"
KIOSK_MODE=false

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
  if [ "$( jq -rc 'has("user_locale")' < $WHIST_JSON_FILE )" == "true"  ]; then
    USER_LOCALE="$(jq -rc '.user_locale | to_entries[] | "\(.key)=\(.value)"' < $WHIST_JSON_FILE)"
    USER_LOCALE="${USER_LOCALE//$'\n'/ }"
  fi
  if [ "$( jq -rc 'has("system_languages")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SYSTEM_LANGUAGES="$(jq -rc '.system_languages' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("browser_languages")' < $WHIST_JSON_FILE )" == "true"  ]; then
    BROWSER_LANGUAGES="$(jq -rc '.browser_languages' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("initial_key_repeat")' < $WHIST_JSON_FILE )" == "true"  ]; then
    if [ "$( jq -rc 'has("key_repeat")' < $WHIST_JSON_FILE )" == "true"  ]; then
      INITIAL_KEY_REPEAT=$( jq -rc '.initial_key_repeat' < $WHIST_JSON_FILE )
      KEY_REPEAT=$( jq -rc '.key_repeat' < $WHIST_JSON_FILE )
      # Set key repeat rate and repeat delay
      xset r rate "$INITIAL_KEY_REPEAT" "$KEY_REPEAT"
    fi
  fi
  if [ "$( jq -rc 'has("initial_url")' < $WHIST_JSON_FILE )" == "true"  ]; then
    INITIAL_URL="$(jq -rc '.initial_url' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("user_agent")' < $WHIST_JSON_FILE )" == "true"  ]; then
    USER_AGENT="$(jq -rc '.user_agent' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("kiosk_mode")' < $WHIST_JSON_FILE )" == "true"  ]; then
    KIOSK_MODE="$(jq -rc '.kiosk_mode' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("latitude")' < $WHIST_JSON_FILE )" == "true"  ]; then
    LATITUDE="$(jq -rc '.latitude' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("longitude")' < $WHIST_JSON_FILE )" == "true"  ]; then
    LONGITUDE="$(jq -rc '.longitude' < $WHIST_JSON_FILE)"
  fi
fi

# Most keys on macOS do not repeat, but all keys repeat on Linux. We turn off key repeat on certain Linux keys
# to match the macOS behavior. This needs to be done *after* setting the key repeat rate above.
# We have decided to allow numbers to repeat because, even though Macs don't repeat numbers,
# they do repeat things like exclamation marks.
# Keycodes obtianed from: https://gist.github.com/rickyzhang82/8581a762c9f9fc6ddb8390872552c250#file-keycode-linux-L91
keys_to_turn_off_repeat=(
  9   # ESC
  67  # F1
  68  # F2
  69  # F3
  70  # F4
  71  # F5
  72  # F6
  73  # F7
  74  # F8
  75  # F9
  76  # F10
  95  # F11
  96  # F12
  78  # Scroll Lock
  110 # Pause
  106 # Insert
  77  # Num Lock
  24  # Q
  25  # W
  26  # E
  27  # R
  28  # T
  29  # Y
  30  # U
  31  # I
  32  # O
  33  # P
  66  # Caps Lock
  38  # A
  39  # S
  40  # D
  41  # F
  42  # G
  43  # H
  44  # J
  45  # K
  46  # L
  52  # Z
  53  # X
  54  # C
  55  # V
  56  # B
  57  # N
  58  # M
  62  # Shift Right
  37  # Ctrl Left
  115 # Logo Left (-> Option)
  64  # Alt Left (-> Command)
  110 # Alt Right
  117 # Menu (-> International)
  109 # Ctrl Right
  50 # Shift Left
)
for keycode in "${keys_to_turn_off_repeat[@]}"
do
  xset -r "$keycode"
done

# Set all JSON transport-related settings
# We set the TZ environment variable (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
# in order to automatically adjust the timezone at the lower layers
export TZ=$DESIRED_TIMEZONE
export DARK_MODE=$DARK_MODE
export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
export INITIAL_URL=$INITIAL_URL
export USER_AGENT="$USER_AGENT"
export KIOSK_MODE=$KIOSK_MODE
export LONGITUDE=$LONGITUDE
export LATITUDE=$LATITUDE
export SENTRY_ENVIRONMENT=${SENTRY_ENVIRONMENT:-}
export USER_LOCALE=$USER_LOCALE
export SYSTEM_LANGUAGES=$SYSTEM_LANGUAGES
export BROWSER_LANGUAGES=$BROWSER_LANGUAGES


exec runuser --login whist --whitelist-environment=TZ,DARK_MODE,RESTORE_LAST_SESSION,INITIAL_URL,USER_AGENT,KIOSK_MODE,SENTRY_ENVIRONMENT,LONGITUDE,LATITUDE,USER_LOCALE,SYSTEM_LANGUAGES,BROWSER_LANGUAGES -c \
  'DISPLAY=:10 \
    LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
    LOCAL=yes \
    LC_ALL=C \
    PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  DEBIAN_FRONTEND=noninteractive '"$1"
