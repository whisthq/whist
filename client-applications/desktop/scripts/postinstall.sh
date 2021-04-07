#!/bin/bash

cd node_modules/@fractal/core-ts && npm install && npm run build

cd ../..

cd electron-notarize && yarn && yarn build

