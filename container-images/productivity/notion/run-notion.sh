#!/bin/bash

pushd "/opt/notion/" >/dev/null || exit 1
./electron app.asar $@
popd >/dev/null || exit 1
