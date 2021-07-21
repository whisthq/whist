#!/bin/bash

set -Eeuo pipefail

cd node_modules/@fractal/core-ts && yarn install && yarn run build
