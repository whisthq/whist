#!/bin/bash

if [[ -f "protocol-build/client/Fractal" ]]; then
    echo "Protocol built, starting..."
else
    echo "Protocol not built! Installing it first..."
    sh ./scripts/publish.sh
fi

snowpack dev
