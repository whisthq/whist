#!/bin/bash

set -Eeuo pipefail

pushd node_modules/@whist/core-ts
yarn install
yarn build
popd

yarn tailwind
yarn assets:pull
