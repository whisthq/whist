#!/bin/bash

set -Eeuo pipefail

cd node_modules/@whist/core-ts && yarn install && yarn build
