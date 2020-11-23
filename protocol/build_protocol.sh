#!/bin/bash

set -Eeuo pipefail

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

git_hash=$(git rev-parse --short HEAD)
if [[ ${1:-''} == release ]]; then
    release_tag='-DCMAKE_BUILD_TYPE=Release'
else
    release_tag=''
fi

# build protocol
# note: we clean build to prevent cmake caching issues, for example when
# switching container base from Ubuntu 18 to Ubuntu 20 and back
(cd "$DIR" && ./docker-create-builder.sh)
(cd "$DIR" && ./docker-run-builder-shell.sh \
    $(pwd)/.. \
    " \
    git clean -dfx -- protocol && \
    cd protocol &&
    cmake . ${release_tag} && \
    make clang-format && \
    make -j FractalServer \
    "
  )
