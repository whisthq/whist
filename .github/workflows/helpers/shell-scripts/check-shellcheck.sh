#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

ALL_SH_FILES="-name '*.sh'"
FIND_ARGS=${1:-$ALL_SH_FILES}


while read file
do
  if shellcheck $file; then
    echo "[Shellcheck passed] $file"
  else
    echo "[Shellcheck did not pass] $file" && exit 1
  fi
done < <(eval find . $FIND_ARGS)
