#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Process command-line args
DIR_NAME=$1
FILE_NAME=$2

while read -r line; do
  case $line in
    "Watches established.")
      echo "Checking if $FILE_NAME already exists in $DIR_NAME..."
      [[ -f $DIR_NAME/$FILE_NAME ]] && echo "Yes, it does!" && break
      ;;
    $FILE_NAME)
      echo "$FILE_NAME has been created!"
      break
      ;;
  esac
done < <(inotifywait -me create,moved_to --format="%f" $DIR_NAME 2>&1)
