#!/bin/bash

# This script will take a diff of files in standard input
# And, it will take a list of "trigger" folders as an argument
# If any file in the diff, is a part of the list of trigger folders,
# then we return successfully. Else, we return unsuccessfully

IFS="|"
PATHS="$*"
NUM_MATCHES=$(grep -E "^($PATHS)" | wc -l)
# If no matches in the diff are found, then we return unsuccessfully
if [ "$NUM_MATCHES" == "0" ]; then
  exit 1
# Otherwise, at least one file in the path has actually been changed,
# so we return successfully
else
  exit 0
fi

