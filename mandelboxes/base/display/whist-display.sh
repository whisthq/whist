#!/bin/bash

# Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
eval "$(sentry-cli bash-hook)"

# Exit on subcommand errors
set -Eeuo pipefail

# Launch xinit
nohup /usr/bin/xinit -- /usr/bin/Xorg :${whist_display_number} -noreset +extension GLX +extension RANDR +extension RENDER -logfile ${whist_xorg_log_file} -config ${whist_xorg_conf_file} vt${whist_tty_number} &

# Wait for xinit-pid to be written to, which happens at the bottom of xinitrc
while [[ ! -f /usr/share/fractal/xinit-pid ]]; do
  sleep 0.01;
done

# Exit, to tell systemd that all is well
