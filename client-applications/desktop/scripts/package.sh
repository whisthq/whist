#!/usr/bin/env bash

yarn build && electron-builder build --config electron-builder.config.js  --publish never
