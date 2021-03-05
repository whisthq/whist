#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

while read file
do
    if grep -q \
            -e "^set -Eeuo pipefail$" \
            -e "^# @fractal-skip-check-pipefail$" \
            "$file"
    then
        echo "[set -Eeuo pipefail -> Present] $file"
    else
        echo "[set -Eeuo pipefail -> NOT FOUND] $file" && exit 1
    fi
done < <(find . -name '*.sh')
