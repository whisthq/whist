#!/bin/bash

set -Eeuo pipefail

git_hash=$(git rev-parse --short HEAD)
if [[ ${1:-''} == release ]]; then
    release_tag='-DCMAKE_BUILD_TYPE=Release'
else
    release_tag=''
fi

# build protocol
# note: we clean build to prevent cmake caching issues, for example when
# switching container base from Ubuntu 18 to Ubuntu 20 and back
./docker-create-builder.sh
./docker-run-builder-shell.sh \
    $(pwd) \
    " \
    cd base/protocol && \
    git clean -dfx && \
    cmake . ${release_tag} && \
    make clang-format && \
    make -j FractalServer \
    "
