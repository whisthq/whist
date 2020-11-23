#!/bin/bash

set -Eeuo pipefail

# Function to build a specific image
# Usage: build_specific_image $app_path $tag $always_show_output
# e.g. build_specific_image browsers/firefox current-build false
# If always_show_output is false, then the build output is shown only on error
build_specific_image () {
    if [[ $# != 3 ]]
    then
        echo "build_specific_image(): Missing an argument!"
        exit -1
    fi

    printf "Building image: %s...\n" "$1:$2"

    if [[ $3 == "true" ]]
    then
        docker build -f $1/Dockerfile.20 $1 -t fractal/$1:$2
    else
        set +e
        docker build -f $1/Dockerfile.20 $1 -t fractal/$1:$2 >> .build_output 2>&1
        status=$?
        set -e

        if [[ $status != 0 ]]
        then
            printf "\tError building image %s!! Build output:\n%s" "$1:$2" "$(cat .build_output)"
            rm .build_output
            exit $status
        else
            printf "\tSuccessfully built image %s.\n" "$1:$2"
            rm .build_output
        fi
    fi
}


# Function to build an image and its dependencies from the Dockerfile
# Usage: build_image_with_deps $app_path $tag $always_show_output
# e.g. build_image_with_deps creative/figma false
# If always_show_output is false, then the build output is shown only on error
build_image_with_deps() {
    if [[ $# != 3 ]]
    then
        echo "build_image_with_deps(): Missing an argument!"
        exit -1
    fi

    if [[ $1 == "base" ]]
    then
        # Deal with base case
        build_specific_image "$1" "$2" "$3"
    else
        # Get the fractal images that are also dependencies
        deps_with_tags=$(grep -o -E "^[[:blank:]]*FROM[[:blank:]]+fractal[^[:blank:]]*" $1/Dockerfile.20 | sed "s/[[:blank:]]*FROM[[:blank:]]\+fractal\///g")

        # We do this nested echo thing to get rid of newlines
        echo "Dependencies for $1: " $(echo $deps_with_tags)

        # Actually build the dependencies
        while IFS= read -r tagged_dep
        do
            if [[ $tagged_dep == *:* ]]
            then
                # There exists a tag
                dep=$(echo "$tagged_dep" | sed "s/:.*//g")
                tag=$(echo "$tagged_dep" | sed "s/^.*://g")
            else
                # We use the default tag "latest"
                dep="$tagged_dep"
                tag="latest"
            fi

            build_image_with_deps "$dep" "$tag" "$3"
        done < <(printf "%s\n" "$deps_with_tags")

        # Build the final image
        build_specific_image "$1" "$2" "$3"
    fi
}


# Input variables and constants
local_tag=current-build
app_path=${1:-base}
app_path=${app_path%/}
force_output=${2:-false}

build_image_with_deps "$app_path" "$local_tag" "$force_output"
