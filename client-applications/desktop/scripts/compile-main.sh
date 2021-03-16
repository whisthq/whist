#!/usr/bin/env bash

esbuild src/main/index.ts  \
    --bundle \
    --platform=node \
    --outdir=dist \
    --external:electron
