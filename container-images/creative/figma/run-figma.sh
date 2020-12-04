#!/bin/bash

# close all instances of chrome
# (to simplify login redirection story)
if pgrep chrome; then
    pkill chrome
fi

# open figma
pushd "/opt/figma/" >/dev/null || exit 1
exec ./electron app.asar $@
popd >/dev/null || exit 1
