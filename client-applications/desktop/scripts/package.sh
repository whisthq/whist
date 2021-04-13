#!/bin/bash

set -Eeuo pipefail

publish=${publish:-never}

set DEBUG=electron-builder && VERSION=$(git describe --abbrev=0) snowpack build && electron-builder build --config electron-builder.config.js  --publish $publish
