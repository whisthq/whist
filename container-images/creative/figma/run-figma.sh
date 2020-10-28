#!/bin/bash

pushd "/opt/figma/" >/dev/null || exit 1
./electron app.asar $@
popd >/dev/null || exit 1
