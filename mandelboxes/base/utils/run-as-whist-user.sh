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
# DARK_MODE=false
#RESTORE_LAST_SESSION=true # unused
#LOAD_EXTENSION=false # make a development flag
DESIRED_TIMEZONE=Etc/UTC # needs to be set on system
# INITIAL_KEY_REPEAT=68 # default value on macOS, options are 120, 94, 68, 35, 25, 15 # needs to be set on system
# KEY_REPEAT=6 # default value on macOS, options are 120, 90, 60, 30, 12, 6, 2 # needs to be set on system
#INITIAL_URL="" # unused
#USER_AGENT="" # already set by extension
SYSTEM_LANGUAGES="en_US" # needs to be set on system
#KIOSK_MODE=false # always should be on kiosk - TODO: make a development flag
CLIENT_OS="darwin" # set to mac for now

# # Most keys on macOS do not repeat, but all keys repeat on Linux. We turn off key repeat on certain Linux keys
# # to match the macOS behavior. This needs to be done *after* setting the key repeat rate above.
# # We have decided to allow numbers to repeat because, even though Macs don't repeat numbers,
# # they do repeat things like exclamation marks.
# # Keycodes obtianed from: https://gist.github.com/rickyzhang82/8581a762c9f9fc6ddb8390872552c250#file-keycode-linux-L91
# keys_to_turn_off_repeat=(
#   9   # ESC
#   67  # F1
#   68  # F2
#   69  # F3
#   70  # F4
#   71  # F5
#   72  # F6
#   73  # F7
#   74  # F8
#   75  # F9
#   76  # F10
#   95  # F11
#   96  # F12
#   78  # Scroll Lock
#   110 # Pause
#   106 # Insert
#   77  # Num Lock
#   24  # Q
#   25  # W
#   26  # E
#   27  # R
#   28  # T
#   29  # Y
#   30  # U
#   31  # I
#   32  # O
#   33  # P
#   66  # Caps Lock
#   38  # A
#   39  # S
#   40  # D
#   41  # F
#   42  # G
#   43  # H
#   44  # J
#   45  # K
#   46  # L
#   52  # Z
#   53  # X
#   54  # C
#   55  # V
#   56  # B
#   57  # N
#   58  # M
#   62  # Shift Right
#   37  # Ctrl Left
#   115 # Logo Left (-> Option)
#   64  # Alt Left (-> Command)
#   110 # Alt Right
#   117 # Menu (-> International)
#   109 # Ctrl Right
#   50 # Shift Left
# )
# for keycode in "${keys_to_turn_off_repeat[@]}"
# do
#   xset -r "$keycode"
# done

# Set all JSON transport-related settings
# We set the TZ environment variable (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
# in order to automatically adjust the timezone at the lower layers
export TZ=$DESIRED_TIMEZONE
# export DARK_MODE=$DARK_MODE
# export RESTORE_LAST_SESSION=$RESTORE_LAST_SESSION
# export LOAD_EXTENSION=$LOAD_EXTENSION
# export INITIAL_URL=$INITIAL_URL
# export USER_AGENT="$USER_AGENT"
# export KIOSK_MODE=$KIOSK_MODE
export SENTRY_ENVIRONMENT=${SENTRY_ENVIRONMENT:-}
export SYSTEM_LANGUAGES=$SYSTEM_LANGUAGES
export CLIENT_OS=$CLIENT_OS

exec runuser --login whist --whitelist-environment=TZ,RESTORE_LAST_SESSION,LOAD_EXTENSION,INITIAL_URL,USER_AGENT,KIOSK_MODE,SENTRY_ENVIRONMENT,SYSTEM_LANGUAGES,CLIENT_OS -c \
  'DISPLAY=:10 \
    LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:/usr/local/nvidia/lib:/usr/local/nvidia/lib64 \
    LOCAL=yes \
    LC_ALL=C \
    PATH=/usr/local/cuda-11.0/bin:/usr/local/nvidia/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  DEBIAN_FRONTEND=noninteractive '"$1"
