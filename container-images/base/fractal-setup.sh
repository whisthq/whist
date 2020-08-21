#!/bin/bash
function set-vars() {
	export DISPLAY=$(cat /home/fractal/displayenv)
	touch /home/fractal/.Xauthority
	export XAUTHORITY=/home/fractal/.Xauthority
}