#!/bin/bash

# This script launches a PulseAudio server associated with the Whist display

# # Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
eval "$(sentry-cli bash-hook)"

# # Exit on subcommand errors
set -Eeuo pipefail

# Launch PulseAudio
/usr/bin/pulseaudio --system --daemonize=no --exit-idle-time=-1 --disallow-exit
