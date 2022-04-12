#!/bin/bash

# This script runs the mouse_events.py script for a specified amount of time

# Exit on subcommand errors
set -Eeuo pipefail

if [[ "$#" -ne 1  ||  ! ( "$1" =~ ^[0-9]+$ ) ]]; then
  echo "Usage: ./simulate_mouse_scrolling.sh <integer number of repeats>"
  exit 1
fi
repeats=$1

echo "Simulating the mouse scroll events in the client $repeats times..."

for (( i=1; i<=repeats; i++ ))
do
  sleep 5
  echo "Mouse scroll events - round $i"
  python3 /usr/share/whist/mouse_events.py
done

echo "Mouse scroll simulation complete."
