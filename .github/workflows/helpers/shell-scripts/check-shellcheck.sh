#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

ALL_SH_FILES="-name '*.sh'"
FIND_ARGS=${1:-$ALL_SH_FILES}


while read -r file
do
  if shellcheck -e SC2028 "$file"; then
    echo "[Shellcheck passed] $file"
  else
    echo "[Shellcheck did not pass] $file" && exit 1
  fi
done < <(eval find . "$FIND_ARGS")
