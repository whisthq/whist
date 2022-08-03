#!/bin/bash

# This script starts the Whist protocol client (only runs on Linux)

# Exit on subcommand errors
set -Eeuo pipefail


SERVER_IP_ADDRESS=127.0.0.1
SERVER_PORT_MAPPINGS=""
SERVER_AES_KEY=""
INITIAL_URLS=""

WHIST_MAPPINGS_DIR=/whist/resourceMappings
SESSION_ID_FILENAME=$WHIST_MAPPINGS_DIR/session_id
WHIST_LOGS_FOLDER=/var/log/whist

# Read the session id, if the file exists
if [ -f "$SESSION_ID_FILENAME" ]; then
  SESSION_ID=$(cat $SESSION_ID_FILENAME)
  WHIST_LOGS_FOLDER=$WHIST_LOGS_FOLDER/$SESSION_ID
fi

# To avoid interfering with Filebeat, the logs files should not contain hyphens in the name before the {-out, -err}.log suffix
PROTOCOL_OUT_FILENAME=$WHIST_LOGS_FOLDER/protocol_client-out.log
PROTOCOL_ERR_FILENAME=$WHIST_LOGS_FOLDER/protocol_client-err.log


# Sample JSON: {"dev_client_server_ip": "35.170.79.124", "dev_client_server_port_mappings": "32262:19020.32263:17242.32273:29843", "dev_client_server_aes_key": "70512c062ff1101f253be70e4cac81bc"}
WHIST_JSON_FILE=/whist/resourceMappings/config.json
if [[ -f $WHIST_JSON_FILE ]]; then
  if [ "$( jq -rc 'has("dev_client_server_ip")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_IP_ADDRESS="$(jq -rc '.dev_client_server_ip' < $WHIST_JSON_FILE)"
  fi
  if [ "$( jq -rc 'has("dev_client_server_port_mappings")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_PORT_MAPPINGS="$(jq -rc '.dev_client_server_port_mappings' < $WHIST_JSON_FILE)"
  else
    echo "Server port mappings not found in JSON data!"
    exit 1
  fi
  if [ "$( jq -rc 'has("dev_client_server_aes_key")' < $WHIST_JSON_FILE )" == "true"  ]; then
    SERVER_AES_KEY="$(jq -rc '.dev_client_server_aes_key' < $WHIST_JSON_FILE)"
  else
    echo "Server AES key not found in JSON data!"
    exit 1
  fi
  if [ "$( jq -rc 'has("initial_urls")' < $WHIST_JSON_FILE )" == "true"  ]; then
    INITIAL_URLS="$(jq -rc '.initial_urls' < $WHIST_JSON_FILE)"
  else
    echo "Initial URL(s) not found in JSON data!"
  fi
fi

# Use pipe to pass arguments to the protocol to be able to open multiple URLs
( (
    cat <<EOF
ports?$SERVER_PORT_MAPPINGS
private-key?$SERVER_AES_KEY
server-ip?$SERVER_IP_ADDRESS
finished
open-url?$INITIAL_URLS
EOF
); sleep 240; echo "quit" ) | /usr/share/whist/WhistClient --dynamic-arguments > $PROTOCOL_OUT_FILENAME 2>$PROTOCOL_ERR_FILENAME &

# `sleep <N second>; echo "quit"` will turn off the client once we are done gathering metrics data. The value here (240) will be replaced by the protocol/test/helpers/whist_client_tools.py script
whist_client_pid=$!

# TODO: once our mandelboxes have bash 5.1 we will be able to deduce _which_
# application exited with the `-p` flag to `wait`.
wait -n
echo "WhistClient exited with code $?"
echo "WhistClient PID: $whist_client_pid"
