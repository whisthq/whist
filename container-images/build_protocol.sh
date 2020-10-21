#!/bin/bash

set -Eeuo pipefail

git_hash=$(git rev-parse --short HEAD)

# build protocol
# note: we clean build to prevent cmake caching issues, for example when
# switching container base from Ubuntu 18 to Ubuntu 20 and back
( cd base/protocol && ./docker-create-builder.sh )
base/protocol/docker-run-builder-shell.sh \
    $(pwd) \
    ''' \
    cd base/protocol && \
    git clean -dfx && \
    cmake . && \
    make clang-format && \
    make -j \
'''
