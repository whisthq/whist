#!/bin/bash

git_hash=$(git rev-parse --short HEAD)

# build protocol
( cd base/protocol && ./docker-build-image.sh $2 )
base/protocol/docker-shell.sh \
    $2 \
    $(pwd) \
    ''' \
    cd base/protocol && \
    cmake . && \
    make -j \
'''

# build container with protocol inside it
docker build -f $1/Dockerfile.$2 $1 -t fractal-$1:$git_hash.$2
