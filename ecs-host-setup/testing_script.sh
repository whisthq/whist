#!/bin/bash

set -Eeuo pipefail

if [ "$EUID" -eq 0 ]; then
    echo "This script cannot be run as root!"
    exit
fi

# Parse input variables
GH_PAT=${1}
GH_USERNAME=${2} 
GIT_BRANCH=${3}
GIT_HASH=${4}

echo $GH_PAT
echo $GH_USERNAME
echo $GIT_BRANCH
echo $GIT_HASH