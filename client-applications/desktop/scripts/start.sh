#!/usr/bin/env bash

# nodemon \
#     --watch "./src/main" \
#     --watch "./src/utils" \
#     --ext js,jsx,ts,tsx \
#     --exec "yarn compile-both"
concurrently \
    "snowpack dev" \
    "electron build/dist/main"
