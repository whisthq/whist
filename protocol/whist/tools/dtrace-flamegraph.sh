#!/bin/bash

set -Eeuo pipefail

if [[ $EUID -ne 0 ]]; then
  echo "$0 is not running as root. Try using sudo."
  exit 2
fi

if [[ ! $# -eq 1 ]]; then
  echo "Error: No PID specified!"
  echo "Usage: $0 TARGET_PID"
  exit 1
fi

SRC_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
FREQUENCY=1001hz

dtrace -x ustackframes=100 -n "profile-$FREQUENCY /pid == $1/ { @[ustack()] = count(); } tick-60s { exit(0); }" \
  | "$SRC_DIR"/flamegraph-helpers/stackcollapse.pl \
  | "$SRC_DIR"/flamegraph-helpers/flamegraph.pl \
  > graph.svg
