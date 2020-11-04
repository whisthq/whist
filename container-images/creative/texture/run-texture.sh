#!/bin/bash

pushd "/opt/texturelab/" >/dev/null || exit 1
yarn electron:serve
popd >/dev/null || exit 1

