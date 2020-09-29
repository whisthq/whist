#!/usr/bin/env bash

i3 &
parcellite -d -n &

# exec gimp

# running with login means that we lose all environment variables, so we need to pass them in manually
exec runuser --login fractal -c 'DISPLAY=:10 google-chrome --start-maximized'
