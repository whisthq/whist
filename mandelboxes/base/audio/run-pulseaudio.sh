#!/bin/bash

# # Enable Sentry bash error handler, this will catch errors if `set -e` is set in a Bash script
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

# # Exit on subcommand errors
set -Eeuo pipefail

# Wait for the PulseAudio unix socket to be active
# Uses 5% CPU until pulseaudio activates
while ! pactl list | grep UNIX; do
  sleep 0.01
done

pulseaudio_watchdog() {
  # Periodically check that the pulseaudio daemon exists
  while pidof pulseaudio; do
    sleep 10
  done

  exit 1
}

# Run the watchdog
pulseaudio_watchdog &

# Write the pid of the watchdog into the pidfile for the systemd service
echo "$!" > /usr/share/whist/whist-audio-pid

# Detatch the watchdog
disown

### SET CLIENT SESSION ID ON SERVER LOGS ###
# Note: Its necessary to this before the `whist-main.service` starts
# because otherwise we won't be able to modify the logs path.
WHIST_MAPPINGS_DIR=/whist/resourceMappings/
block-until-file-exists.sh $WHIST_MAPPINGS_DIR/session_id >&1

# Get the session id from the file written by the host service
SESSION_ID=$(cat $WHIST_MAPPINGS_DIR/session_id)

# Create a directory with the session id where the service
# logs will be sent to. We need this path structure so the
# session id can be parsed by filebeat.
mkdir "/var/log/whist/$SESSION_ID/"

cat > /etc/systemd/system/whist-main.service.d/output.conf << EOF
[Service]
StandardOutput=file:/var/log/whist/$SESSION_ID/protocol-out.log
StandardError=file:/var/log/whist/$SESSION_ID/protocol-err.log
EOF

echo "Set whist main service unit file to $SESSION_ID"

systemctl daemon-reload

# Exit, to tell systemd that all is well
