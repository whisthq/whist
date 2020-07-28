#!/bin/bash
export DISPLAY=$(cat /home/fractal/displayenv)
touch /home/fractal/.Xauthority
export XAUTHORITY=/home/fractal/.Xauthority