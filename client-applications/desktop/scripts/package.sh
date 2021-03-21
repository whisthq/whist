#!/usr/bin/env bash

publish=${publish:-never}

yarn build && electron-builder build --config electron-builder.config.js  --publish $publish
