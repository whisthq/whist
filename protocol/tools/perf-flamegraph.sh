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
FREQUENCY=1001 # Hz
PERF_FILE=perf.data # default

perf record -F $FREQUENCY -p $1 -g --call-graph dwarf -- sleep 60

perf script \
  | "$SRC_DIR"/flamegraph-helpers/stackcollapse-perf.pl \
  | "$SRC_DIR"/flamegraph-helpers/flamegraph.pl \
  > graph.svg

rm $PERF_FILE
