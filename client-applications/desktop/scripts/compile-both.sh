#!/usr/bin/env bash
set -euo pipefail

concurrently \
    --kill-others \
    "snowpack dev" \
    "yarn compile-main && electron dist"
