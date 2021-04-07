#!/bin/bash

set -Eeuo pipefail

publish=${publish:-never}

set DEBUG=electron-builder && yarn build && electron-builder build --config electron-builder.config.js  --publish $publish
