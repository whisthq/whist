#!/bin/bash

# close all instances of chrome
# (to simplify login redirection story)
if pgrep chrome; then
    pkill chrome
fi

# open slack
exec /usr/lib/slack/slack $@