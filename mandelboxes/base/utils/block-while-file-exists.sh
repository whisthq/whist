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
      echo "Checking if $FILE_NAME already does NOT exist in $DIR_NAME..."
      [[ ! -f $DIR_NAME/$FILE_NAME ]] && echo "This file does not exist!" && break
      ;;
    $FILE_NAME)
      echo "$FILE_NAME has been deleted!"
      break
      ;;
  esac
done < <(inotifywait -me delete,moved_from --format="%f" "$DIR_NAME" 2>&1)
