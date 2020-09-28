#!/usr/bin/env bash

i3 &
parcellite -d -n &

# exec gimp

exec google-chrome --no-sandbox --start-maximized
