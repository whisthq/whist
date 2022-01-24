#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Process command-line args
DIR_NAME="$(dirname "$1")"
FILE_NAME="$(basename "$1")"

while read -r line; do
  echo "inotify event: $line"
  case $line in
    "Watches established.")
      echo "inotify event: Checking if $FILE_NAME already exists in $DIR_NAME..."
      [[ -f $DIR_NAME/$FILE_NAME ]] && echo "inotify event: $FILE_NAME exists!" && break
      ;;
    $FILE_NAME)
      echo "inotify event: $FILE_NAME has been created!"
      break
      ;;
  esac
done < <(inotifywait -me create,moved_to --format="%f" $DIR_NAME 2>&1)
