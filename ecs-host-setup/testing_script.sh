#!/bin/bash

set -Eeuo pipefail

# Parse input variables
GH_PAT=${1}
GH_USERNAME=${2} 
GIT_BRANCH=${3}
GIT_HASH=${4}

ghcr_uri=ghcr.io
echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri