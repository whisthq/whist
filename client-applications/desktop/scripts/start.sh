#!/bin/bash

set -Eeuo pipefail

if [[ -f "protocol-build/client/ Fractal" ]]; then
    echo "Protocol built, starting..."
else
    echo "Protocol not built! Installing it first..."
    ./scripts/publish.sh
fi

yarn tailwind
snowpack dev
