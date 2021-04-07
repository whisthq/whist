#!/bin/bash

set -Eeuo pipefail

cd node_modules/@fractal/core-ts && npm install && npm run build
