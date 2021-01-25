#!/bin/bash

# get relative subfolder path
# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Find all the Dockerfiles
dockerfile_paths=()
find . -name "Dockerfile.20" -print0 >tmpfile
while IFS=  read -r -d $'\0'; do
    dockerfile_paths+=("$REPLY")
done <tmpfile
rm -f tmpfile

# Parse Dockerfiles to get the proper format for GitHub Container Registry
apps=()
for path in "${dockerfile_paths[@]}"
do
    :
    path=${path%/Dockerfile.20}
    path=${path#"./"}
    apps+=($path)
done

# output images in repo
printf '%s ' "${apps[@]}"
