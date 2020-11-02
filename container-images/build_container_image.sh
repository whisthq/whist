#!/bin/bash

set -Eeuo pipefail

local_tag=current-build

app_path=${1:-base}
app_path=${app_path%/}

if [[ $app_path == "base" ]]
then
  echo "Building base image..."
  docker build -f $app_path/Dockerfile.20 $app_path -t fractal/$app_path:$local_tag
else
  # Get the fractal images that are also dependencies
  deps_with_tags=$(grep -o -E "^[[:blank:]]*FROM[[:blank:]]+fractal[^[:blank:]]*" $app_path/Dockerfile.20 | sed "s/[[:blank:]]*FROM[[:blank:]]\+fractal\///g")

  while IFS= read -r tagged_dep
  do
    untagged_dep=$(echo "$tagged_dep" | sed "s/:.*//g")
    echo "Building dependency $untagged_dep ($tagged_dep)..."

    docker build -f $untagged_dep/Dockerfile.20 $untagged_dep -t fractal/$tagged_dep >> .build_output 2>&1
    status=$?

    if [[ $status != 0 ]]
    then
      printf "\tError building dependency %s (%s)!! Output:\n%s" "$untagged_dep" "$tagged_dep" $(cat .build_output)
      exit $status
    else
      printf "\tSuccessfully built dependency %s (%s).\n" "$untagged_dep" "$tagged_dep"
    fi

    rm .build_output

  done < <(printf "%s\n" "$deps_with_tags")

  docker build -f $app_path/Dockerfile.20 $app_path -t fractal/$app_path:$local_tag
fi
