#!/bin/bash

pushd "/opt/notion-app/" >/dev/null || exit 1
./electron app $@
popd >/dev/null || exit 1
