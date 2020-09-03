#!/bin/bash

set -o errexit
set -o pipefail
set -o nounset

export PATH=$PATH:/usr/local/bin

if ! which hadolint &>/dev/null; then
    >&2 echo 'hadolint command not found'
    if [ -z "$(command -v brew)" ]; then
        exit 1
    fi
    brew install hadolint
fi

hadolint "$@"
