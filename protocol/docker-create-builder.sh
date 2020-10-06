#!/bin/bash
docker build --build-arg uname=$(whoami) --build-arg uid=$(id -u ${USER}) \
            --build-arg gid=$(id -g ${USER}) -f Dockerfile.builder.ubuntu20 \
            . -t protocol-builder-ubuntu20
