#!/bin/bash

pushd "/opt/texturelab/" >/dev/null || exit 1
yarn install && yarn electron:serve
popd >/dev/null || exit 1

